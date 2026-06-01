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
#include "HistoManager.hh"
#include "PrimaryGeneratorAction.hh"
#include "Constants.hh"

#include "G4ProcessTable.hh"
#include "G4Radioactivation.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

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

void Run::ParticleCount(G4String name, G4double Ekin, G4double meanLife)
{
  std::map<G4String, ParticleData>::iterator it = fParticleDataMap1.find(name);
  if (it == fParticleDataMap1.end()) {
    fParticleDataMap1[name] = ParticleData(1, Ekin, Ekin, Ekin, meanLife);
  }
  else {
    ParticleData& data = it->second;
    data.fCount++;
    data.fEmean += Ekin;
    // update min max
    G4double emin = data.fEmin;
    if (Ekin < emin) data.fEmin = Ekin;
    G4double emax = data.fEmax;
    if (Ekin > emax) data.fEmax = Ekin;
    data.fTmean = meanLife;
  }
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

void Run::Merge(std::map<G4String, ParticleData>& destinationMap,
                const std::map<G4String, ParticleData>& sourceMap) const
{
  for (const auto& particleData : sourceMap) {
    G4String name = particleData.first;
    const ParticleData& localData = particleData.second;
    if (destinationMap.find(name) == destinationMap.end()) {
      destinationMap[name] = ParticleData(localData.fCount, localData.fEmean, localData.fEmin,
                                          localData.fEmax, localData.fTmean);
    }
    else {
      ParticleData& data = destinationMap[name];
      data.fCount += localData.fCount;
      data.fEmean += localData.fEmean;
      G4double emin = localData.fEmin;
      if (emin < data.fEmin) data.fEmin = emin;
      G4double emax = localData.fEmax;
      if (emax > data.fEmax) data.fEmax = emax;
      data.fTmean = localData.fTmean;
    }
  }
}

void Run::Merge(const G4Run* run)
{
  const Run* localRun = static_cast<const Run*>(run);

  // primary particle info
  fParticle = localRun->fParticle;
  fEkin = localRun->fEkin;

  // map: processes count
  for (const auto& procCounter : localRun->fProcCounter) {
    G4String procName = procCounter.first;
    G4int localCount = procCounter.second;
    if (fProcCounter.find(procName) == fProcCounter.end()) {
      fProcCounter[procName] = localCount;
    }
    else {
      fProcCounter[procName] += localCount;
    }
  }

  // map: created particles count
  Merge(fParticleDataMap1, localRun->fParticleDataMap1);

  G4Run::Merge(run);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::EndOfRun()
{
  G4int prec = 5, wid = prec + 2;
  G4int dfprec = G4cout.precision(prec);

  // run condition
  // G4Material* material = fDetector->GetAbsorberMaterial();

  if (numberOfEvent == 0) {
    G4cout.precision(dfprec);
    return;
  }

  // G4String Particle = fParticle ? fParticle->GetParticleName() : "unknown";
  // G4cout << "\n The run is " << numberOfEvent << " " << Particle << "(s) through "
        //  << G4BestUnit(fDetector->GetAbsorberThickness(), "Length") << " of " << material->GetName()
        //  << " (density: " << G4BestUnit(material->GetDensity(), "Volumic Mass") << ")" << G4endl;

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

  // particles count
  G4cout << G4endl;
  G4cout << "============================================================" << G4endl;
  G4cout << "      List of generated particles (with meanLife != 0)      " << G4endl;
  G4cout << "============================================================" << G4endl;

  for (const auto& particleData : fParticleDataMap1) {
    G4String name = particleData.first;
    ParticleData data = particleData.second;
    G4int count = data.fCount;
    G4double eMean = data.fEmean / count;
    G4double eMin = data.fEmin;
    G4double eMax = data.fEmax;
    G4double meanLife = data.fTmean;

    if (meanLife > 0.)
    {
      G4cout << "  " << std::setw(13) << name << ": " << std::setw(7) << count
            << "  Emean = " << std::setw(wid) << G4BestUnit(eMean, "Energy") << "\t( "
            << G4BestUnit(eMin, "Energy") << " --> " << G4BestUnit(eMax, "Energy") << ")";
      G4cout << "\tmean life = " << G4BestUnit(meanLife, "Time") << G4endl;
    }
  }

  WriteActivity(numberOfEvent);

  // remove all contents in fProcCounter, fCount
  fProcCounter.clear();
  // fParticleDataMap.clear();
  fParticleDataMap1.clear();
  // fParticleDataMap2.clear();

  // restore default format
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
