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
#include "G4AnalysisManager.hh"

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
  //========================================================================//

  // Initialize analysis manager
  G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("root");
  analysisManager->SetFileName(fFileName);
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);


  //========================================================================//
  // Create ntuple for event-by-event analysis
  //========================================================================//

  // Energy deposition (id = 0)
  analysisManager->CreateNtuple("EnergyDeposition", "Gamma energy deposition in the detectors");
  analysisManager->CreateNtupleDColumn("EnergyDep");  // Column 0: Energy deposited in the detector (MeV)
  analysisManager->CreateNtupleDColumn("Weight");     // Column 1: Weight associated with the energy deposition (for weighted events)
  analysisManager->CreateNtupleDColumn("Time");       // Column 2: Global time of the step (us)
  analysisManager->FinishNtuple();  // Finalize

  // Emitted particle information (id = 1)
  analysisManager->CreateNtuple("EmittedParticles", "Particles emitted from the sample");
  analysisManager->CreateNtupleIColumn("PID");        // Column 0: Particle ID (PDG code)
  analysisManager->CreateNtupleDColumn("Energy");     // Column 2: Energy of the emitted particle (MeV)
  analysisManager->CreateNtupleDColumn("Weight");     // Column 3: Weight associated with the emitted particle (for weighted events)
  analysisManager->CreateNtupleDColumn("Time");       // Column 4: Global time of the emission (us)
  analysisManager->FinishNtuple();  // Finalize

  // Decay product information (id = 2)
  analysisManager->CreateNtuple("DecayProducts", "Decay products in the sample");
  analysisManager->CreateNtupleIColumn("PID");              // Column 0: Particle ID (PDG code)
  analysisManager->CreateNtupleIColumn("Z");                // Column 1: Atomic number of the decay product
  analysisManager->CreateNtupleIColumn("A");                // Column 2: Mass number of the decay product
  analysisManager->CreateNtupleSColumn("CreatorProcess");   // Column 3: Name of the process that created the decay product
  analysisManager->CreateNtupleDColumn("ExcitationEnergy"); // Column 5: Excitation energy of the decay product (MeV)
  analysisManager->CreateNtupleDColumn("Weight");           // Column 6: Weight associated with the decay product (for weighted events)
  analysisManager->CreateNtupleIColumn("IsStable");         // Column 7: Flag indicating if the decay product is stable (1 for stable, 0 for unstable)
  analysisManager->CreateNtupleDColumn("TimeBirth");        // Column 7: Global time of the decay product's birth (us)
  analysisManager->CreateNtupleDColumn("TimeDeath");        // Column 8: Global time of the decay product's death (us)
  analysisManager->FinishNtuple();  // Finalize

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
