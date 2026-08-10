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
/// \file SteppingAction.cc
/// \brief Implementation of the SteppingAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "SteppingAction.hh"

#include "DetectorConstruction.hh"
#include "EventAction.hh"
#include "HistoManager.hh"
#include "Run.hh"

#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

SteppingAction::SteppingAction(DetectorConstruction* det, EventAction* event)
  : fDetector(det), fEventAction(event)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Count physics processes and record per-step energy deposits
//------------------------------------------------------------------------//
void SteppingAction::UserSteppingAction(const G4Step* aStep)
{
  // Get the current run and analysis manager
  Run* run = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());

  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

  // Count processes
  const G4StepPoint* endPoint = aStep->GetPostStepPoint();
  const G4VProcess* process = endPoint->GetProcessDefinedStep();
  run->CountProcesses(process);

  // Cache the HPGe volumes
  if (!fActiveCrystalLV) {
    fActiveCrystalLV = fDetector->GetActiveCrystalLV();
  }

  // Get energy deposit for this step
  G4double edep = aStep->GetTotalEnergyDeposit();
  if (edep <= 0.) return;

  // Get volume of pre-step point
  const G4VPhysicalVolume* pv = aStep->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
  if (!pv) return;
  G4LogicalVolume* lv = pv->GetLogicalVolume();

  // Check if step is in either HPGe
  if (lv != fActiveCrystalLV) return;

  // Record energy deposits in NTuple
  G4double time = aStep->GetPreStepPoint()->GetGlobalTime();
  G4double weight = aStep->GetPreStepPoint()->GetWeight();

  analysisManager->FillNtupleDColumn(0, HistoManager::kNT_EnergyDep, edep);
  analysisManager->FillNtupleDColumn(0, HistoManager::kNT_EDepWeight, weight);
  analysisManager->FillNtupleDColumn(0, HistoManager::kNT_EDepTime, time / microsecond);
  analysisManager->AddNtupleRow(0);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
