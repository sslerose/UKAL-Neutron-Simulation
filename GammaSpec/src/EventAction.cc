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
// * Geant4 License and Disclaimer                                    *
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
// * technical work of the Geant4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//

//
/// \file EventAction.cc
/// \brief Implementation of the EventAction class
/// Analyzes detector hits and fills histograms
//

#include "EventAction.hh"
#include "DetectorHit.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "HistoManager.hh"
#include "Constants.hh"
#include "Run.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4RunManager.hh"

#include <iomanip>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

EventAction::EventAction(DetectorConstruction* det, PrimaryGeneratorAction* primaryGen) : fDetector(det), fPrimaryGenerator(primaryGen) {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::BeginOfEventAction(const G4Event* event) {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Retrieve hits collection from event by ID
//------------------------------------------------------------------------//
DetectorHitsCollection* EventAction::GetHitsCollection(G4int hcID, const G4Event* event) const
{
  // Retrieve hits collection of given collection ID (sensitive detector) for given event
  auto hitsCollection = static_cast<DetectorHitsCollection*>(
    event->GetHCofThisEvent()->GetHC(hcID));

  if (!hitsCollection) {
    G4ExceptionDescription msg;
    msg << "Cannot access hits collection ID " << hcID << " for current event." << G4endl;
    G4Exception("EventAction::GetHitsCollection()", "NeutronTOF001", EventMustBeAborted, msg);
    return nullptr;
  }

  return hitsCollection;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Analyze sensitive detector hits collection at the end of each event
//------------------------------------------------------------------------//
void EventAction::AnalyzeHits(DetectorHitsCollection* hc)
{
  //========================================================================//
  // Initialize event variables
  //========================================================================//
  
  G4bool neutronEntered = false;    // track if neutron entered detector
  G4bool captureOccurred = false;   // track if neutron capture occurred
  G4double captureTime = -1.0;      // time of neutron capture (-1 = no capture)

  auto analysisManager = G4AnalysisManager::Instance();


  //========================================================================//
  // Get primary neutron parameters from primary generator
  //========================================================================//
  
  G4double neutronEnergy_keV = 0.;
  G4double neutronTheta_deg = 0.;
  
  if (fPrimaryGenerator) {
    neutronEnergy_keV = fPrimaryGenerator->GetNeutronEnergy();
    neutronTheta_deg = fPrimaryGenerator->GetNeutronTheta() * 180.0 / CLHEP::pi;  // Convert theta from radians to degrees for ntuple
  }
  else {
    G4ExceptionDescription msg;
    msg << "PrimaryGeneratorAction pointer is null. "
        << "Check that PrimaryGeneratorAction is properly passed to EventAction in ActionInitialization::Build()." << G4endl;
    G4Exception("EventAction::AnalyzeHits()", "NeutronDet001", RunMustBeAborted, msg);
  }  
  

  //========================================================================//
  // Analyze hits in the detector
  //========================================================================//

  // Get number of hits in this event
  G4int nHits = hc->entries();

  // Loop through all hits in this event
  if (nHits > 0) {
    for (G4int i = 0; i < nHits; i++) {
      DetectorHit* hit = (*hc)[i];

      // Check for neutron entry into detector
      if (hit->GetNeutronEntered() && hit->GetProcessName() == "neutronEntry") {
        neutronEntered = true;
      }
      
      // Check for 6Li(n,t)4He reaction - marks where scintillation occurs
      if (hit->GetProcessName() == "Li6ntalpha") {
        captureTime = hit->GetTime(); // Global time (since event start)
        captureOccurred = true;
        break;  // Exit loop after first capture marker (defensive for edge cases)
      }
    }
  }


  //========================================================================//
  // Fill histograms
  //========================================================================//

  // Generated neutron energy and angle histograms
  analysisManager->FillH1(HistoManager::kH_NeutronEnergy, neutronEnergy_keV);
  analysisManager->FillH1(HistoManager::kH_NeutronTheta, neutronTheta_deg);

  // Number of neutron hits histogram
  analysisManager->FillH1(HistoManager::kH_nHits, nHits);


  //========================================================================//
  // Fill ntuple with event-by-event data
  //========================================================================//

  analysisManager->FillNtupleIColumn(0, HistoManager::kNT_CaptureFlag, captureOccurred ? 1 : 0);
  analysisManager->FillNtupleDColumn(0, HistoManager::kNT_CaptureTime, captureTime / ns);  // -1 if no capture
  analysisManager->AddNtupleRow();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Analyze hits collection at the end of each event
//------------------------------------------------------------------------//
void EventAction::EndOfEventAction(const G4Event* event)
{
  // Track progress across all threads
  Run::RecordEvent(event->GetEventID());

  //========================================================================//
  // Get and analyze hits collection from sensitive detector
  // Called at end of each event
  //========================================================================//

  // Get sensitive detector hits collection ID (cached after first event)
  if (fDetectorHCID == -1) {
    fDetectorHCID = -1;
    
    // Validate that the collection was found
    if (fDetectorHCID == -1) {
      G4ExceptionDescription msg;
      msg << "Hits collection ID for not found. "
          << "Check that DetectorSD is properly called in DetectorConstruction::ConstructSDandField()." << G4endl;
      G4Exception("EventAction::EndOfEventAction()", "NeutronDet001", RunMustBeAborted, msg);
    }
  }

  // Get hits collection of current event
  auto eventHC = event->GetHCofThisEvent();
  if (!eventHC) {
    G4ExceptionDescription msg;
    msg << "No hits collection of current event found." << G4endl;
    G4Exception("EventAction::EndOfEventAction()", "NeutronDet001", EventMustBeAborted, msg);
  }
  
  // Retrieve and analyze sensitive detector hits collection of current event
  auto eventHCOfID = GetHitsCollection(fDetectorHCID, event);
  AnalyzeHits(eventHCOfID);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::PrintEventStatistics(G4double tof, G4int nHits) const
{
  // Print event summary for periodic monitoring
  G4cout
    << "Event Statistics:"
    << "\n  Time of Flight: " << std::setw(7) << G4BestUnit(tof, "Time")
    << "\n  Number of Hits: " << nHits
    << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
