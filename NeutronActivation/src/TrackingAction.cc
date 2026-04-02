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

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

TrackingAction::TrackingAction(EventAction* event) : fEventAction(event) {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PreUserTrackingAction(const G4Track* track)
{
  Run* run = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());

  G4ParticleDefinition* particle = track->GetDefinition();
  G4String name = particle->GetParticleName();
  G4double meanLife = particle->GetPDGLifeTime();
  G4double ekin = track->GetKineticEnergy();
  fTimeBirth = track->GetGlobalTime();

  // count secondary particles (with meanLife > 0)
  if ((track->GetTrackID() > 1) && !(particle->GetPDGStable())) run->ParticleCount(name, ekin, meanLife);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track* track)
{
  Run* run = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());

  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

  const G4ParticleDefinition* particle = track->GetParticleDefinition();
  G4String name = particle->GetParticleName();
  G4double meanLife = particle->GetPDGLifeTime();
  G4double ekin = track->GetKineticEnergy();
  fTimeEnd = track->GetGlobalTime();
  if ((particle->GetPDGStable()) && (ekin == 0.)) fTimeEnd = DBL_MAX;

  // count population of ions with meanLife > 0.
  if ((G4IonTable::IsIon(particle)) && !(particle->GetPDGStable())) {
    G4int id = run->GetIonId(name);
    G4double unit = analysisManager->GetH1Unit(id);
    G4double tmin = analysisManager->GetH1Xmin(id) * unit;
    G4double tmax = analysisManager->GetH1Xmax(id) * unit;
    G4double binWidth = analysisManager->GetH1Width(id) * unit;
    G4double weight = track->GetWeight();

    G4double t1 = std::max(fTimeBirth, tmin);
    G4double t2 = std::min(fTimeEnd, tmax);
    for (G4double time = t1; time < t2; time += binWidth) analysisManager->FillH1(id, time, weight);
    
    analysisManager->FillNtupleSColumn(1, HistoManager::kNT_IonName, name);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_TimeBirth, fTimeBirth / s);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_TimeDeath, fTimeEnd / s);
    analysisManager->FillNtupleDColumn(1, HistoManager::kNT_Weight, weight);
    analysisManager->AddNtupleRow(1);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......