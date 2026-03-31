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
/// \file HistoManager.cc
/// \brief Implementation of the HistoManager class
//

#include "HistoManager.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

#include <string>

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
  //   - SetVerboseLevel(1): Print info about analysis operations
  //   - SetNtupleMerging(true): Combine worker thread ntuples (MT mode)
  //   - SetActivation(true): Enable activation/inactivation of histograms
  //========================================================================//

  // Initialize analysis manager
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);
  analysisManager->SetActivation(true);


  //========================================================================//
  // Create histograms
  //
  // Format: CreateH1("name", "title;xaxis;yaxis", nbins, xmin, xmax)
  //========================================================================//

  // Neutron energy from primary generator (keV)
  analysisManager->CreateH1("NeutronEnergy",
    "Generated Neutron Energy;Energy [keV];Events",
    200, 0., 200.);

  // Neutron angle from primary generator (degrees)
  analysisManager->CreateH1("NeutronTheta",
    "Generated Neutron Angle;Angle [deg];Events",
    90, 0., 90.);

  // Number of neutron hits per event
  analysisManager->CreateH1("NHits",
    "Number of Neutron Hits per Event;Number of Hits;Events",
    10, 0., 10.);
  

  // Default values for population histograms (to be reset via /analysis/h1/set command)
  G4int nbins = 100;
  G4double vmin = 0.;
  G4double vmax = 100.;

  // Population of species
  for (G4int k = 0; k < nID; k++) {
    G4int id = k + kH_nHits + 1;
    G4int ih = analysisManager->CreateH1(std::to_string(id), " population", nbins, vmin, vmax);
    analysisManager->SetH1Activation(ih, false);  // Set inactive by default
  }

  // Time of flight (TOF) - all events (ns)

  // Time of flight (TOF) - capture events only (ns)
  // TOF: time from neutron production to capture
  // analysisManager->CreateH1("TOF",
  //   "Time of Flight (Capture Time);TOF [ns];Events",
  //   300, 0., 300.);

  // // Neutron energy from TOF (keV)
  // analysisManager->CreateH1("TOFEnergy",
  //   "Neutron Energy from TOF;Energy [keV];Events",
  //   150, 0., 150.);


  //========================================================================//
  // Create ntuple for event-by-event analysis
  //
  // CreateNtuple() initializes a new ntuple
  // CreateNtupleDColumn()/CreateNtupleIColumn() add columns
  // FinishNtuple() must be called to finalize the structure
  //========================================================================//
  analysisManager->CreateNtuple("DetectorData", "Neutron Detection Event Data");
  
  // Column 0: Primary neutron energy (keV)
  analysisManager->CreateNtupleDColumn("NeutronEnergy");
  
  // Column 1: Primary neutron angle (degrees)
  analysisManager->CreateNtupleDColumn("NeutronTheta");

  // Column 2: Entry flag (1 = entered detector, 0 = did not enter)
  analysisManager->CreateNtupleIColumn("EntryFlag");

  // Column 3: Capture flag (1 = capture occurred, 0 = no capture)
  analysisManager->CreateNtupleIColumn("CaptureFlag");
  
  // // Column 4: Time of flight = capture time (ns), -1 if no capture
  // analysisManager->CreateNtupleDColumn("TOF");

  // // Column 5: Neutron energy from TOF (keV), -1 if no capture
  // analysisManager->CreateNtupleDColumn("TOFEnergy");
  
  
  analysisManager->FinishNtuple();

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
