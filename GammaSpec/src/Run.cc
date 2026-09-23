//
// ********************************************************************
// * Q-SNAC License and Disclaimer                                    *
// *                                                                  *
// * This file is part of a Geant4-based simulation of neutron        *
// * capture experiments via scintillation developed by Sam LeRose    *
// * under the supervision of Dr. Ruchi Mahajan at the University of  *
// * Kentucky Accelerator Laboratory.                                 *
// *                                                                  *
// * This code is provided under the terms and conditions of the MIT  *
// * License, a copy of which is available at                         *
// * https://opensource.org/license/mit .                             *
// *                                                                  *
// * Portions of this work are based on existing Geant4 examples and  *
// * tutorials.  Their respective license and disclaimer statements   *
// * are included below.                                              *
// *                                                                  *
// ********************************************************************
//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//

//
/// \file Run.cc
/// \brief Implementation of the Run class
//

#include "Run.hh"

#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4ProcessTable.hh"
#include "G4Radioactivation.hh"
#include "G4AnalysisManager.hh"

#include <cmath>
#include <ctime>

namespace
{
// Mutex to lock updating the global ion map
G4Mutex ionIdMapMutex = G4MUTEX_INITIALIZER;
}

// Static member definitions for progress tracking
std::atomic<G4int> Run::fGlobalEventCount{0};
std::atomic<G4int> Run::fNextGlobalMilestone{0};
G4int Run::fTotalEventsInRun = 0;
G4int Run::fCurrentMilestonePercent = 0;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::Run(DetectorConstruction* det) : fDetector(det) {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::SetPrimary(G4ParticleDefinition* particle, G4double energy)
{
  fParticle = particle;
  fEkin = energy;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::CountProcesses(const G4VProcess* process)
{
  if (process == nullptr) return;
  G4String procName = process->GetProcessName();
  std::map<G4String, G4int>::iterator it = fProcCounter.find(procName);
  if (it == fProcCounter.end()) {
    fProcCounter[procName] = 1;
  }
  else {
    fProcCounter[procName]++;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::ParticleCount(G4String name, G4int Z, G4int A)
{
  // Increment total ion count
  fIonTrackCount++;

  // Increment distinct isomer count, keyed by full name (includes excitation state)
  fIsomerCounter[name]++;

  // Increment distinct nuclide count, keyed by Z and A (collapse isomers)
  G4String nuclideKey = std::to_string(Z) + "_" + std::to_string(A);
  fNuclideCounter[nuclideKey]++;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Initialize progress tracking at the start of the run
//------------------------------------------------------------------------//
void Run::InitProgressTracking(G4int totalEvents)
{
  fTotalEventsInRun = totalEvents;
  fGlobalEventCount.store(0);
  fCurrentMilestonePercent = 10;
  if (totalEvents > 0) {
    fNextGlobalMilestone.store(static_cast<G4int>(std::round(0.1 * totalEvents)));
  } else {
    fNextGlobalMilestone.store(0);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Reset progress tracking (e.g. between runs in the same job)
//------------------------------------------------------------------------//
void Run::ResetProgressTracking()
{
  fGlobalEventCount.store(0);
  fNextGlobalMilestone.store(0);
  fTotalEventsInRun = 0;
  fCurrentMilestonePercent = 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Check event count and print progress if milestone reached
//------------------------------------------------------------------------//
void Run::RecordEvent(G4int /*eventID*/)
{
  if (fTotalEventsInRun <= 0) return;

  G4int count = fGlobalEventCount.fetch_add(1) + 1;

  G4int milestone = fNextGlobalMilestone.load();
  if (milestone > 0 && count >= milestone) {
    // Try to claim this milestone via CAS
    if (fNextGlobalMilestone.compare_exchange_strong(milestone, 0)) {
      // Get system wall-clock time
      std::time_t now = std::time(nullptr);
      std::tm* localTime = std::localtime(&now);
      char timeStr[64];
      std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", localTime);

      G4cout << ">>> Event " << milestone << " starts at time " << timeStr << G4endl;

      // Compute next milestone
      G4int nextPercent = fCurrentMilestonePercent + 10;
      if (nextPercent < 100) {
        fCurrentMilestonePercent = nextPercent;
        G4int nextMilestone = static_cast<G4int>(
          std::round(static_cast<double>(nextPercent) / 100.0 * fTotalEventsInRun));
        fNextGlobalMilestone.store(nextMilestone);
      }
      // If nextPercent >= 100, leave fNextGlobalMilestone at 0 (no more milestones)
    }
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Merge counters from the local run into the global run
//------------------------------------------------------------------------//
void Run::Merge(const G4Run* run)
{
  const Run* localRun = static_cast<const Run*>(run);

  // primary particle info
  fParticle = localRun->fParticle;
  fEkin = localRun->fEkin;

  // Ion track totals
  fIonTrackCount += localRun->fIonTrackCount;

  // Processes
  for (const auto& procCounter : localRun->fProcCounter) {
    G4String procName = procCounter.first;
    G4int localCount = procCounter.second;

    fProcCounter[procName] += localCount;
  }

  // Nuclides
  for (const auto& nuclideCounter : localRun->fNuclideCounter) {
    G4String nuclideKey = nuclideCounter.first;
    G4int localCount = nuclideCounter.second;

    fNuclideCounter[nuclideKey] += localCount;
  }

  // Isomers
  for (const auto& isomerCounter : localRun->fIsomerCounter) {
    G4String isomerName = isomerCounter.first;
    G4int localCount = isomerCounter.second;

    fIsomerCounter[isomerName] += localCount;
  }

  G4Run::Merge(run);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::EndOfRun()
{
  G4int prec = 5, wid = prec + 2;
  G4int dfprec = G4cout.precision(prec);

  if (numberOfEvent == 0) {
    G4cout.precision(dfprec);
    return;
  }

  // frequency of processes
  G4cout << G4endl;
  G4cout << "============================================================" << G4endl;
  G4cout << "                  Process calls frequency                   " << G4endl;
  G4cout << "============================================================" << G4endl;
  std::map<G4String, G4int>::iterator it;
  for (it = fProcCounter.begin(); it != fProcCounter.end(); it++) {
    G4String procName = it->first;
    G4int count = it->second;
    G4cout << "\t" << procName << "= " << count;
  }
  G4cout << G4endl;

  auto* p = G4ProcessTable::GetProcessTable()->FindProcess("Radioactivation", "GenericIon");
  G4cout << "dynamic_cast G4Radioactivation*: "
        << dynamic_cast<G4Radioactivation*>(p) << G4endl;

  // Ion track summary
  G4cout << G4endl;
  G4cout << "============================================================" << G4endl;
  G4cout << "                      Ion Track Summary                     " << G4endl;
  G4cout << "============================================================" << G4endl;
  G4cout << "   Total ion tracks recorded: " << fIonTrackCount << G4endl;
  G4cout << "   Distinct nuclides generated: " << fNuclideCounter.size() << G4endl;
  G4cout << "   Distinct isomers generated: " << fIsomerCounter.size() << G4endl;

  G4cout << G4endl;
  G4cout << "   Top " << std::min(5, (int)fNuclideCounter.size()) << " nuclides generated:" << G4endl;

  std::vector<std::pair<G4String, G4int>> sortedNuclides(fNuclideCounter.begin(), fNuclideCounter.end());
  std::sort(sortedNuclides.begin(), sortedNuclides.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

  for (size_t i = 0; i < std::min<size_t>(5, sortedNuclides.size()); ++i) {
    const auto& nuclide = sortedNuclides[i];
    G4cout << "      " << nuclide.first << ": " << nuclide.second << G4endl;
  }

  G4cout << G4endl;
  G4cout << "   Full per-track breakdown available in ROOT output." << G4endl;

  WriteActivity(numberOfEvent);

  // Remove contents in counters
  fProcCounter.clear();
  fNuclideCounter.clear();
  fIsomerCounter.clear();

  // Restore default format
  G4cout.precision(dfprec);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::WriteActivity(G4int nevent)
{
  G4ProcessTable* pTable = G4ProcessTable::GetProcessTable();
  G4Radioactivation* rDecay =
    (G4Radioactivation*)pTable->FindProcess("Radioactivation", "GenericIon");

  // output the induced radioactivities (in VR mode only)
  //
  if (rDecay == 0) {
    G4cerr << "WriteActivity: G4Radioactivation process not found for GenericIon." << G4endl;
    return;
  }
  if (rDecay->IsAnalogueMonteCarlo()) {
    G4cerr << "WriteActivity: Running in analogue MC mode — no activity file written." << G4endl;
    return;
  }

  G4String fileName = G4AnalysisManager::Instance()->GetFileName() + ".activity";
  std::ofstream outfile(fileName, std::ios::out);

  std::vector<G4RadioactivityTable*> theTables = rDecay->GetTheRadioactivityTables();

  for (size_t i = 0; i < theTables.size(); i++) {
    G4double rate, error;
    outfile << "Radioactivities in decay window no. " << i << G4endl;
    outfile << "Z \tA \tE \tActivity (decays/window) \tError (decays/window) " << G4endl;

    map<G4ThreeVector, G4TwoVector>* aMap = theTables[i]->GetTheMap();
    map<G4ThreeVector, G4TwoVector>::iterator iter;
    for (iter = aMap->begin(); iter != aMap->end(); iter++) {
      rate = iter->second.x() / nevent;
      error = std::sqrt(iter->second.y()) / nevent;
      if (rate < 0.) rate = 0.;  // statically it can be < 0.
      outfile << iter->first.x() << "\t" << iter->first.y() << "\t" << iter->first.z() << "\t"
              << rate << "\t" << error << G4endl;
    }
    outfile << G4endl;
  }
  outfile.close();
}
