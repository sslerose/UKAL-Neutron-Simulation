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

#include "HistoManager.hh"
#include "Run.hh"

#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PreUserTrackingAction(const G4Track*)
{
  //========================================================================//
  // Initialize counters at start of each track
  //========================================================================//
  fNbStepNonThermal = 0;
  fNbStepThermal = 0;
  fTrackLenNonThermal = 0.;
  fTrackLenThermal = 0.;
  fTimeNonThermal = 0.;
  fTimeThermal = 0.;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::UpdateTrackInfo(G4double ekin, G4double trackLength, G4double time)
{
  const G4double thermalEnergy = 1.0 * eV;
  
  if (ekin > thermalEnergy) {
    // Non-thermal (fast) neutron regime
    fNbStepNonThermal++;
    fTrackLenNonThermal = trackLength;
    fTimeNonThermal = time;
  }
  else {
    // Thermal neutron regime
    fNbStepThermal++;
    fTrackLenThermal = trackLength - fTrackLenNonThermal;
    fTimeThermal = time - fTimeNonThermal;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track* track)
{
  // // keep only primary neutron
  
  // G4int trackID = track->GetTrackID();
  // if (trackID > 1) return;

  // Run* run = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());
  // run->SumTrackLength(fNbStep1, fNbStep2, fTrackLen1, fTrackLen2, fTime1, fTime2);

  // // histograms
  
  // G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  // analysisManager->FillH1(1, fNbStep1);
  // analysisManager->FillH1(2, fTrackLen1);
  // analysisManager->FillH1(3, fTime1);
  // analysisManager->FillH1(4, fNbStep2);
  // analysisManager->FillH1(5, fTrackLen2);
  // analysisManager->FillH1(6, fTime2);

  //========================================================================//
  // Only process primary neutron (TrackID == 1)
  //========================================================================//
  if (track->GetTrackID() != 1) return;

  //========================================================================//
  // Update Run statistics
  //========================================================================//
  Run* run = static_cast<Run*>(
    G4RunManager::GetRunManager()->GetNonConstCurrentRun());
  
  if (run) {
    run->SumTrackLength(fNbStepNonThermal, fNbStepThermal,
                        fTrackLenNonThermal, fTrackLenThermal,
                        fTimeNonThermal, fTimeThermal);
  }

  //========================================================================//
  // Fill diagnostic histograms (IDs 10-13)
  //========================================================================//
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  
  G4double totalTrackLength = fTrackLenNonThermal + fTrackLenThermal;
  G4int totalCollisions = fNbStepNonThermal + fNbStepThermal;
  
  // ID 10: Total track length (convert to cm for readability)
  analysisManager->FillH1(HistoManager::kH_totTrackLen, totalTrackLength / cm);
  
  // ID 11: Total number of collisions
  analysisManager->FillH1(HistoManager::kH_totCollisions, totalCollisions);
  
  // ID 12: Non-thermal track length
  analysisManager->FillH1(HistoManager::kH_trackLenNonThermal, fTrackLenNonThermal / cm);

  // ID 13: Thermal track length
  analysisManager->FillH1(HistoManager::kH_trackLenThermal, fTrackLenThermal / cm);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
