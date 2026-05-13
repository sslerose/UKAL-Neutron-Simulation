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
/// \file PhysicsList.cc
/// \brief Implementation of the PhysicsList class
//

#include "PhysicsList.hh"

#include "G4BaryonConstructor.hh"
#include "G4BosonConstructor.hh"
#include "G4IonConstructor.hh"
#include "G4LeptonConstructor.hh"
#include "G4MesonConstructor.hh"
#include "G4ShortLivedConstructor.hh"

#include "ElectromagneticPhysics.hh"
#include "GammaNuclearPhysics.hh"
#include "HadronElasticPhysicsHP.hh"
#include "RadioactiveDecayPhysics.hh"

#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4EmStandardPhysics.hh"
#include "G4EmExtraPhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4HadronElasticPhysics.hh"
#include "G4HadronPhysicsFTFP_BERT_HP.hh"
#include "G4IonElasticPhysics.hh"
#include "G4StoppingPhysics.hh"
#include "G4NuclideTable.hh"
#include "G4RadioactiveDecayPhysics.hh"
#include "G4IonPhysicsXS.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PhysicsList::PhysicsList()
{
  G4int verb = 1;
  SetVerboseLevel(verb);

  // Add new units
  new G4UnitDefinition("mm2/g", "mm2/g", "Surface/Mass", mm2 / g);
  new G4UnitDefinition("um2/mg", "um2/mg", "Surface/Mass", um * um / mg);

  const G4double meanLife = 1 * nanosecond, halfLife = meanLife * std::log(2);
  G4NuclideTable::GetInstance()->SetThresholdOfHalfLife(halfLife);

  //========================================================================//
  // Hadron elastic physics
  //========================================================================//
  RegisterPhysics(new HadronElasticPhysicsHP(verb));

  //========================================================================//
  // Hadron inelastic physics
  //========================================================================//
  RegisterPhysics(new G4HadronPhysicsFTFP_BERT_HP(verb));

  //========================================================================//
  // Ion elastic physics
  //========================================================================//
  RegisterPhysics(new G4IonElasticPhysics(verb));

  //========================================================================//
  // Ion inelastic physics
  //========================================================================//
  RegisterPhysics(new G4IonPhysicsXS(verb));

  //========================================================================//
  // Stopping physics
  //========================================================================//
  RegisterPhysics(new G4StoppingPhysics(verb));

  //========================================================================//
  // Gamma-nuclear physics
  //========================================================================//
  RegisterPhysics(new GammaNuclearPhysics("gamma"));

  //========================================================================//
  // Electromagnetic physics
  //========================================================================//
  RegisterPhysics(new ElectromagneticPhysics());
  
  //========================================================================//
  // Decay physics - for unstable particles
  //========================================================================//
  RegisterPhysics(new G4DecayPhysics(verb));
  
  //========================================================================//
  // Radioactive decay physics
  //========================================================================//
  RegisterPhysics(new RadioactiveDecayPhysics());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PhysicsList::ConstructParticle()
{
  G4BosonConstructor pBosonConstructor;
  pBosonConstructor.ConstructParticle();

  G4LeptonConstructor pLeptonConstructor;
  pLeptonConstructor.ConstructParticle();

  G4MesonConstructor pMesonConstructor;
  pMesonConstructor.ConstructParticle();

  G4BaryonConstructor pBaryonConstructor;
  pBaryonConstructor.ConstructParticle();

  G4IonConstructor pIonConstructor;
  pIonConstructor.ConstructParticle();

  G4ShortLivedConstructor pShortLivedConstructor;
  pShortLivedConstructor.ConstructParticle();
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PhysicsList::SetCuts()
{
  //========================================================================//
  // Production thresholds for secondary particles
  // These cuts define the minimum range for producing secondaries
  //========================================================================//

  // Set production cuts to zero for protons to ensure accurate tracking
  // for neutron generation from Li(p,n) reactions
  SetCutValue(0 * mm, "proton");
  
  // Default cuts for other particles
  SetCutValue(10 * km, "e-");
  SetCutValue(10 * km, "e+");
  SetCutValue(10 * km, "gamma");
  
  if (verboseLevel > 0) {
    DumpCutValuesTable();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
