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
/// \file NeutronHPphysics.cc
/// \brief Implementation of the NeutronHPphysics class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "NeutronHPphysics.hh"

#include "G4GenericMessenger.hh"
#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessTable.hh"

// Processes
#include "G4HadronElasticProcess.hh"
#include "G4HadronInelasticProcess.hh"
#include "G4NeutronCaptureProcess.hh"
#include "G4NeutronFissionProcess.hh"
#include "G4ParticleHPCapture.hh"
#include "G4ParticleHPCaptureData.hh"
#include "G4ParticleHPElastic.hh"
#include "G4ParticleHPElasticData.hh"
#include "G4ParticleHPFission.hh"
#include "G4ParticleHPFissionData.hh"
#include "G4ParticleHPInelastic.hh"
#include "G4ParticleHPInelasticData.hh"
#include "G4ParticleHPThermalScattering.hh"
#include "G4ParticleHPThermalScatteringData.hh"
#include "G4SystemOfUnits.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

NeutronHPphysics::NeutronHPphysics(const G4String& name) : G4VPhysicsConstructor(name)
{
  // define commands for this class
  DefineCommands();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

NeutronHPphysics::~NeutronHPphysics()
{
  delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void NeutronHPphysics::ConstructProcess()
{
  G4ParticleDefinition* neutron = G4Neutron::Neutron();
  G4ProcessManager* pManager = neutron->GetProcessManager();

  //========================================================================//
  // Delete default neutron processes from PhysicsList
  // Replaced with high-precision (HP) models below
  //========================================================================//

  G4VProcess* process = 0;
  process = pManager->GetProcess("hadElastic");
  if (process) pManager->RemoveProcess(process);
  
  process = pManager->GetProcess("neutronInelastic");
  if (process) pManager->RemoveProcess(process);
  
  process = pManager->GetProcess("nCapture");
  if (process) pManager->RemoveProcess(process);
  
  process = pManager->GetProcess("nFission");
  if (process) pManager->RemoveProcess(process);


  //========================================================================//
  // NEUTRON ELASTIC SCATTERING (with thermal scattering)
  //========================================================================//
  // Includes both regular elastic scattering (> 4 eV) and 
  // thermal scattering (< 4 eV)

  // Process
  G4HadronElasticProcess* theElasticProcess = new G4HadronElasticProcess();
  pManager->AddDiscreteProcess(theElasticProcess);

  // Cross section data set
  theElasticProcess->AddDataSet(new G4ParticleHPElasticData());

  // Model
  G4ParticleHPElastic* theElasticModel = new G4ParticleHPElastic();
  theElasticProcess->RegisterMe(theElasticModel);

  // Thermal scattering model for neutrons below 4 eV (if enabled)
  if (fThermal) {
    // Set minimum energy for regular elastic model
    theElasticModel->SetMinEnergy(4 * eV);

    // Model
    G4ParticleHPThermalScattering* theThermalModel = new G4ParticleHPThermalScattering();
    theElasticProcess->RegisterMe(theThermalModel);

    // Set maximum energy for thermal scattering model
    theThermalModel->SetMaxEnergy(4 * eV);

    // Cross section data set
    theElasticProcess->AddDataSet(new G4ParticleHPThermalScatteringData());
  }


  //========================================================================//
  // NEUTRON INELASTIC SCATTERING
  //========================================================================//
  // Primary detection mechanism for neutrons
  // MT = 700 for 6Li(n,t)alpha reaction

  // Process
  G4HadronInelasticProcess* theInelasticProcess =
    new G4HadronInelasticProcess("neutronInelastic", G4Neutron::Definition());
  pManager->AddDiscreteProcess(theInelasticProcess);
  
  // Cross section data set
  theInelasticProcess->AddDataSet(new G4ParticleHPInelasticData());
  
  // Model
  theInelasticProcess->RegisterMe(new G4ParticleHPInelastic());


  //========================================================================//
  // NEUTRON CAPTURE
  //========================================================================//
  // Radiative capture (n,gamma)

  // Process
  G4NeutronCaptureProcess* theCaptureProcess = new G4NeutronCaptureProcess();
  pManager->AddDiscreteProcess(theCaptureProcess);
  
  // Cross section data sets
  theCaptureProcess->AddDataSet(new G4ParticleHPCaptureData());
  
  // Model
  theCaptureProcess->RegisterMe(new G4ParticleHPCapture());


  //========================================================================//
  // NEUTRON FISSION
  //========================================================================//
  // For completeness

  // Process
  G4NeutronFissionProcess* theFissionProcess = new G4NeutronFissionProcess();
  pManager->AddDiscreteProcess(theFissionProcess);
  
  // Cross section data set
  theFissionProcess->AddDataSet(new G4ParticleHPFissionData());
  
  // Model
  theFissionProcess->RegisterMe(new G4ParticleHPFission());

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void NeutronHPphysics::DefineCommands()
{
  // Define /neutronTOF/phys command directory using generic messenger class
  // Changed from /testhadr/phys/ to match the rest of the project
  fMessenger = new G4GenericMessenger(this, "/neutronTOF/phys/", "physics list commands");

  // thermal scattering command
  auto& thermalCmd = fMessenger->DeclareProperty("thermalScattering", fThermal);

  thermalCmd.SetGuidance("Set thermal scattering model (true/false)");
  thermalCmd.SetParameterName("thermal", false);
  thermalCmd.SetStates(G4State_PreInit);
}

//..oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......