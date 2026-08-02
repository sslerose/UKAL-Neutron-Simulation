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
/// \file TrackingAction.cc
/// \brief Implementation of the TrackingAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "TrackingAction.hh"

#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "HistoManager.hh"
#include "Run.hh"

#include "G4IonTable.hh"
#include "G4ParticleTypes.hh"
#include "G4RunManager.hh"
#include "G4StepStatus.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4UnitsTable.hh"
#include "G4HadronicProcessType.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TrackingAction::TrackingAction(DetectorConstruction* det, EventAction* event)
  : fDetectorConstruction(det), fEventAction(event)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PreUserTrackingAction(const G4Track* track)
{
  // Get the current run and analysis manager
  Run* run = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());

  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

  // Get track information
  G4ParticleDefinition* particle = track->GetDefinition();
  G4int pdgCode = particle->GetPDGEncoding();
  G4String name = particle->GetParticleName();
  G4double charge = particle->GetPDGCharge();
  G4double kineticEnergy = track->GetKineticEnergy();
  G4double meanLife = particle->GetPDGLifeTime();
  G4double time   = track->GetGlobalTime();   // birth time
  G4double weight = track->GetWeight();

  G4bool isIon = G4IonTable::IsIon(particle);
  G4double excitationEnergy = 0.0;
  if (isIon) {
    const G4Ions* ion = dynamic_cast<const G4Ions*>(particle);
    excitationEnergy = ion->GetExcitationEnergy();
  }


  //========================================================================//
  // Record emission spectrum data
  //========================================================================//

  // Get the process that created the track (nullptr for primaries)
  const G4VProcess* creator = track->GetCreatorProcess();

  // Check if the track is a product of a radioactive decay process and has charge < 3
  if (creator && creator->GetProcessSubType() == fRadioactiveDecay && charge < 3.){
    analysisManager->FillNtupleIColumn(1, HistoManager::kNT_EmissionPID, pdgCode);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_EmEnergy, kineticEnergy);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_EmWeight, weight);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_EmTime, time / microsecond);
    analysisManager->AddNtupleRow(1);
  }


  //========================================================================//
  // Get decay-product inventory
  //========================================================================//

  // Check if the track is within the absorber assembly (including daughter volumes)
  // auto* touchable = track->GetTouchable();
  // G4bool inAbsorberRegion = false;

  // for (G4int depth = 0; depth < touchable->GetHistoryDepth() + 1; ++depth) {
  //   if (touchable->GetVolume(depth)->GetLogicalVolume()->GetName() == "AbsorberAssembly") {
  //       inAbsorberRegion = true;
  //       break;
  //   }
  // }

  // Flag unstable ions
  // G4bool unstableIon = (charge > 2.) && !particle->GetPDGStable();
  
  // If ion in absorber region, record its properties for later use in PostUserTrackingAction
  if (isIon && track->GetTrackID() != 1) {
    G4LogicalVolume* lv = track->GetVolume()->GetLogicalVolume();
    G4bool inAbsorberRegion = (lv == fDetectorConstruction->GetAbsorberLV()) || (lv == fDetectorConstruction->GetGoldLV());

    if (inAbsorberRegion) {
      fRecordTrack = true;
      fTrackPID = pdgCode;
      fTrackZ = particle->GetAtomicNumber();
      fTrackA = particle->GetAtomicMass();
      // fTrackKineticEnergy = kineticEnergy;
      fTrackExcitationEnergy = excitationEnergy;
      fTrackWeight = weight;
      fTimeBirth = time;
      fTrackCreatorProcess = creator ? creator->GetProcessName() : "primary";

      // Record ion data into run object for end of run summary
      run->ParticleCount(name, fTrackZ, fTrackA);
    }
  }

  // count secondary particles (with meanLife > 0)
  // if ((track->GetTrackID() > 1) && !(particle->GetPDGStable())) run->ParticleCount(name, kineticEnergy, meanLife);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track* track)
{
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

  if (!fRecordTrack) return;  // Only record tracks flagged in PreUserTrackingAction

  // Reset the record flag for the next track
  fRecordTrack = false;

  // Record decay product information for unstable ions originally born in absorber region
  G4ParticleDefinition* particle = track->GetDefinition();
  G4double timeEnd = track->GetGlobalTime();  // death time
  G4double kineticEnergy = track->GetKineticEnergy();  // kinetic energy

  // If the track is stable and has zero kinetic energy, set death time to "infinity"
  if ((particle->GetPDGStable()) && (kineticEnergy == 0.)) timeEnd = DBL_MAX;

  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayPID, fTrackPID);
  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayZ, fTrackZ);
  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayA, fTrackA);
  analysisManager->FillNtupleSColumn(2, HistoManager::kNT_DecayCreatorProcess, fTrackCreatorProcess);
  // analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayKineticEnergy, fTrackKineticEnergy);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayExcitationEnergy, fTrackExcitationEnergy);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayWeight, fTrackWeight);
  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayIsStable, particle->GetPDGStable() ? 1 : 0);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayTimeBirth, fTimeBirth / microsecond);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayTimeDeath, timeEnd / microsecond);
  analysisManager->AddNtupleRow(2);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......