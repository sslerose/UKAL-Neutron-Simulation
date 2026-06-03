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

TrackingAction::TrackingAction(EventAction* event) : fEventAction(event) {}

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
  G4double energy = track->GetKineticEnergy();
  G4double meanLife = particle->GetPDGLifeTime();
  G4double time   = track->GetGlobalTime();   // birth time
  G4double weight = track->GetWeight();

  // Record emission spectrum data for radioactive-decay products with charge < 3
  const G4VProcess* creator = track->GetCreatorProcess(); // nullptr for primaries
  if (creator && creator->GetProcessSubType() == fRadioactiveDecay && charge < 3.) {
    analysisManager->FillNtupleIColumn(1, HistoManager::kNT_EmissionPID, pdgCode);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_EmEnergy, energy);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_EmWeight, weight);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_EmTime, time / microsecond);
    analysisManager->AddNtupleRow(1);
  }

  // Get decay-product inventory for unstable ions born in "AbsorberAssembly", excluding primaries
  // Data written in Post action
  G4bool unstableIon = (charge > 2.) && !particle->GetPDGStable();
  const G4VPhysicalVolume* pv = track->GetVolume();
  if (unstableIon && track->GetTrackID() != 1 && pv && pv->GetLogicalVolume()->GetName() == "AbsorberAssembly") {
    fRecordTrack = true;
    fTrackPID = pdgCode;
    fTrackZ = particle->GetAtomicNumber();
    fTrackA = particle->GetAtomicMass();
    fTrackEnergy = energy;
    fTrackWeight = weight;
    fTimeBirth = time;
  }

  // count secondary particles (with meanLife > 0)
  if ((track->GetTrackID() > 1) && !(particle->GetPDGStable())) run->ParticleCount(name, energy, meanLife);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track* track)
{
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

  if (!fRecordTrack) return;  // Only record tracks flagged in PreUserTrackingAction

  // Record decay product information for unstable ions born in "AbsorberAssembly"
  G4ParticleDefinition* particle = track->GetDefinition();
  G4double timeEnd = track->GetGlobalTime();  // death time
  G4double energy = track->GetKineticEnergy();  // kinetic energy

  if ((particle->GetPDGStable()) && (energy == 0.)) timeEnd = DBL_MAX;

  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayPID, fTrackPID);
  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayZ, fTrackZ);
  analysisManager->FillNtupleIColumn(2, HistoManager::kNT_DecayA, fTrackA);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayEnergy, fTrackEnergy);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayWeight, fTrackWeight);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayTimeBirth, fTimeBirth / microsecond);
  analysisManager->FillNtupleDColumn(2, HistoManager::kNT_DecayTimeDeath, timeEnd / microsecond);
  analysisManager->AddNtupleRow(2);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......