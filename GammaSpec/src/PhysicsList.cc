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
/// \file PhysicsList.cc
/// \brief Implementation of the PhysicsList class
//

#include "PhysicsList.hh"

#include "G4EmLivermorePhysics.hh"
#include "G4EmParameters.hh"

#include "G4DeexPrecoParameters.hh"
#include "G4NuclearLevelData.hh"
#include "G4NuclideTable.hh"

#include "G4DecayPhysics.hh"

#include "G4GenericIon.hh"
#include "G4PhysicsListHelper.hh"
#include "G4Radioactivation.hh"

#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PhysicsList::PhysicsList()
{
  G4int verb = 1;
  SetVerboseLevel(verb);

  //=====================================================================//
  // Nuclide table
  //=====================================================================//

  // Set low half-life threshold to capture short-lived states
  G4NuclideTable::GetInstance()->SetThresholdOfHalfLife(0.1 * picosecond);
  // Set minimum energy separation between excited states
  G4NuclideTable::GetInstance()->SetLevelTolerance(1.0 * eV);


  //=====================================================================//
  // Nuclear de-excitation parameters
  //=====================================================================//

  G4DeexPrecoParameters* deex = G4NuclearLevelData::GetInstance()->GetParameters();
  deex->SetStoreICLevelData(true);  // Enable internation conversion model
  deex->SetIsomerProduction(true);  // Enable isomer production

  // Sync deexcitation with nuclide table threshold (takes corresponding mean lifetime of nuclide table half life threshold)
  deex->SetMaxLifeTime(G4NuclideTable::GetInstance()->GetThresholdOfHalfLife() / std::log(2.0));
  

  //=====================================================================//
  // Electromagnetic physics options
  //=====================================================================//
  G4EmParameters* emParams = G4EmParameters::Instance();
  emParams->SetFluo(true);  // Enable fluorescence photon emission
  emParams->SetAugerCascade(true);  // Enable Auger electron emission
  emParams->SetDeexcitationIgnoreCut(true); // Allow de-excitation processes to ignore production cuts
  emParams->AddPhysics("World", "G4Radioactivation"); // Enable radioactive decay processes for all particles


  //=====================================================================//
  // Register physics constructors
  //=====================================================================//
  RegisterPhysics(new G4EmLivermorePhysics());
  RegisterPhysics(new G4DecayPhysics());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PhysicsList::ConstructProcess()
{
  // Build processes for all registered constructors (EM and decay)
  G4VModularPhysicsList::ConstructProcess();

  // Register radioactive decay processes for all particles
  G4PhysicsListHelper::GetPhysicsListHelper()->RegisterProcess(new G4Radioactivation(), G4GenericIon::GenericIon());
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PhysicsList::SetCuts()
{
  //========================================================================//
  // Production thresholds for secondary particles
  // These cuts define the minimum range for producing secondaries
  //========================================================================//

  // Zero cut for protons to for recoil nuclei tracking
  SetCutValue(0 * mm, "proton");
  
  // Low cuts for other particles
  SetCutValue(0.01 * mm, "e-");
  SetCutValue(0.01 * mm, "e+");
  SetCutValue(0.01 * mm, "gamma");
  
  if (verboseLevel > 0) {
    DumpCutValuesTable();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
