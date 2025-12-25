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
/// \file HistoManager.cc
/// \brief Implementation of the HistoManager class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "HistoManager.hh"

#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

HistoManager::HistoManager()
{
  Book();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void HistoManager::Book()
{
  //========================================================================//
  // Create and configure analysis manager
  //
  // Key settings:
  //   - SetDefaultFileType("root"): Output format
  //   - SetNtupleMerging(true): Combine worker thread ntuples (MT mode)
  //   - SetVerboseLevel(1): Print info about analysis operations
  //========================================================================//
  // The choice of analysis technology is done via selection of a namespace
  // in HistoManager.hh
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetFileName(fFileName);
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);   // Merge ntuples from worker threads
  analysisManager->SetActivation(true);  // enable inactivation of histograms


  //========================================================================//
  // Create histograms
  //
  // CreateH1() returns an integer ID (0, 1, 2, ...) that must be used
  // when filling histograms with FillH1(id, value).
  //
  // Format: CreateH1("name", "title;xaxis;yaxis", nbins, xmin, xmax)
  //========================================================================//

  // ID 0: Primary neutron energy from SimLiT (keV)
  // Expected: Peaked distribution depending on proton beam energy
  // For 1912 keV protons, expect ~30-70 keV neutrons at forward angles
  analysisManager->CreateH1("NeutronEnergy",
    "SimLiT Neutron Energy;Energy [keV];Events",
    200, 0., 200.);

  // ID 1: Primary neutron angle from SimLiT (degrees)
  // Expected: Forward-peaked distribution from Li(p,n) kinematics
  analysisManager->CreateH1("NeutronTheta",
    "SimLiT Neutron Angle;Angle [deg];Events",
    90, 0., 90.);

  // ID 2: Total neutrons captured in detector
  analysisManager->CreateH1("NCapture",
    "Total Neutrons Captured per Event;Number of Captures;Events",
    10, 0., 10.);

  // ID 3: Time of flight - capture events only (ns)
  // TOF: time from neutron production to capture
  // Expected: Distribution depends on neutron energy and path length
  // TOF = d/v where v = sqrt(2*E/m_n)
  // For d=51cm and E=50keV: TOF ≈ 1650 ns
  analysisManager->CreateH1("TOF",
    "Time of Flight (Capture Time);TOF [ns];Events",
    300, 0., 300.);

  // ID 4: Number of neutron hits per event
  // Should generally show 0 or 1 hits per event
  analysisManager->CreateH1("NHits",
    "Number of Neutron Hits per Event;Number of Hits;Events",
    10, 0., 10.);

  // ID 5: Neutron energy from TOF (keV)
  analysisManager->CreateH1("TOFEnergy",
    "Neutron Energy from TOF;Energy [keV];Events",
    150, 0., 150.);

  //========================================================================//
  // Create ntuple for event-by-event analysis
  //
  // CreateNtuple() initializes a new ntuple (like a ROOT TTree)
  // CreateNtupleDColumn()/CreateNtupleIColumn() add columns
  // FinishNtuple() must be called to finalize the structure
  //
  // Column IDs are assigned sequentially (0, 1, 2, ...)
  // These IDs are used with FillNtupleDColumn(id, value)
  //========================================================================//
  analysisManager->CreateNtuple("DetectorData", "Neutron Detection Event Data");
  
  // Column 0: Primary neutron energy (keV)
  analysisManager->CreateNtupleDColumn("NeutronEnergy");
  
  // Column 1: Primary neutron angle (degrees)
  analysisManager->CreateNtupleDColumn("NeutronTheta");
  
  // Column 2: Total energy deposited in detector (MeV)
  // analysisManager->CreateNtupleDColumn("TotalEdep");
  
  // Column 3: Time of flight = capture time (ns), -1 if no capture
  analysisManager->CreateNtupleDColumn("TOF");
  
  // Column 4: Capture flag (1 = capture occurred, 0 = no capture)
  analysisManager->CreateNtupleIColumn("CaptureFlag");
  
  analysisManager->FinishNtuple();
  
  //========================================================================//
  // Create diagnostic histograms at IDs 10-13
  // Populated by TrackingAction for primary neutron tracking info
  //========================================================================//
  
  // Need to create placeholder histograms for IDs 5-9 to maintain ID sequence
  // These are inactive placeholders
  for (G4int i = kH_TOFEnergy + 1; i < kH_TOFEnergy + 5; i++) {
    analysisManager->CreateH1("placeholder_" + std::to_string(i), 
                               "placeholder", 10, 0., 1.);
    analysisManager->SetH1Activation(i, false);
  }
  
  // ID 10: Total track length of primary neutron (cm)
  // Useful for understanding effective path length
  analysisManager->CreateH1("DiagTrackLength",
    "Primary Neutron Total Track Length;Track Length [cm];Events",
    200, 0., 100.);

  // ID 11: Total number of collisions (scattering events)
  // Indicates how much scattering occurs before capture
  analysisManager->CreateH1("DiagNCollisions",
    "Primary Neutron Total Collisions;Number of Collisions;Events",
    100, 0., 100.);

  // ID 12: Track length at E > 1 eV (non-thermal)
  // Fast neutron component of transport
  analysisManager->CreateH1("DiagTrackLenNonThermal",
    "Track Length (E > 1 eV);Track Length [cm];Events",
    200, 0., 100.);

  // ID 13: Track length at E < 1 eV (thermal)
  // Thermal neutron component - often where capture occurs
  analysisManager->CreateH1("DiagTrackLenThermal",
    "Track Length (E < 1 eV);Track Length [cm];Events",
    200, 0., 50.);
  
  G4cout << "\n TrackingAction: Created diagnostic histograms (IDs 10-13)"
         << G4endl;

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
