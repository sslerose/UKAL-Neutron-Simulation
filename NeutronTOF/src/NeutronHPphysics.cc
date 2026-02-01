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

// Cross sections
#include "G4NeutronElasticXS.hh"
#include "G4NeutronInelasticXS.hh"
#include "G4NeutronCaptureXS.hh"

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

  // delete all neutron processes if already registered
  //
  G4VProcess* process = 0;
  process = pManager->GetProcess("hadElastic");
  if (process) pManager->RemoveProcess(process);
  //
  process = pManager->GetProcess("neutronInelastic");
  if (process) pManager->RemoveProcess(process);
  //
  process = pManager->GetProcess("nCapture");
  if (process) pManager->RemoveProcess(process);
  //
  process = pManager->GetProcess("nFission");
  if (process) pManager->RemoveProcess(process);


  //========================================================================//
  // NEUTRON ELASTIC SCATTERING (with thermal scattering)
  //========================================================================//
  // Primary transport mechanism through air
  // Includes both regular elastic scattering (> 4 eV) and 
  // thermal scattering (< 4 eV) for thermalized neutrons

  // Process
  G4HadronElasticProcess* theElasticProcess = new G4HadronElasticProcess();
  pManager->AddDiscreteProcess(theElasticProcess);

  // Cross section data set
  theElasticProcess->AddDataSet(new G4ParticleHPElasticData());
  theElasticProcess->AddDataSet(new G4NeutronElasticXS());

  // Model
  G4ParticleHPElastic* theElasticModel = new G4ParticleHPElastic();
  theElasticProcess->RegisterMe(theElasticModel);

  // Thermal scattering (if enabled)
  if (fThermal) {
    // Set minimum energy for regular elastic model
    theElasticModel->SetMinEnergy(4 * eV);

    // Cross section data set
    theElasticProcess->AddDataSet(new G4ParticleHPThermalScatteringData());

    // Model
    theElasticProcess->RegisterMe(new G4ParticleHPThermalScattering());
  }

  // // (re) create process: elastic
  // //
  // G4HadronElasticProcess* process1 = new G4HadronElasticProcess();
  // pManager->AddDiscreteProcess(process1);
  // //
  // // model1a
  // G4ParticleHPElastic* model1a = new G4ParticleHPElastic();
  // process1->RegisterMe(model1a);
  // process1->AddDataSet(new G4ParticleHPElasticData());
  // //
  // // model1b - thermal scattering
  // if (fThermal) {
  //   model1a->SetMinEnergy(4 * eV);
  //   G4ParticleHPThermalScattering* model1b = new G4ParticleHPThermalScattering();
  //   process1->RegisterMe(model1b);
  //   process1->AddDataSet(new G4ParticleHPThermalScatteringData());
  // }


  //========================================================================//
  // NEUTRON INELASTIC SCATTERING
  //========================================================================//

  // Process
  G4HadronInelasticProcess* theInelasticProcess =
    new G4HadronInelasticProcess("neutronInelastic", G4Neutron::Definition());
  pManager->AddDiscreteProcess(theInelasticProcess);
  
  // Cross section data set
  theInelasticProcess->AddDataSet(new G4ParticleHPInelasticData());
  
  // Model
  theInelasticProcess->RegisterMe(new G4ParticleHPInelastic());

  // (re) create process: inelastic
  //
  // G4HadronInelasticProcess* process2 =
  //   new G4HadronInelasticProcess("neutronInelastic", G4Neutron::Definition());
  // pManager->AddDiscreteProcess(process2);
  // //
  // // cross section data set
  // G4ParticleHPInelasticData* dataSet2 = new G4ParticleHPInelasticData();
  // process2->AddDataSet(dataSet2);
  // //
  // // models
  // G4ParticleHPInelastic* model2 = new G4ParticleHPInelastic();
  // process2->RegisterMe(model2);



  //========================================================================//
  // NEUTRON CAPTURE
  //========================================================================//
  // Primary detection mechanism
  // Also captures (n,gamma) reactions in air (N, O)

  // Process
  G4NeutronCaptureProcess* theCaptureProcess = new G4NeutronCaptureProcess();
  pManager->AddDiscreteProcess(theCaptureProcess);
  
  // Cross section data sets
  theCaptureProcess->AddDataSet(new G4ParticleHPCaptureData());
  // theCaptureProcess->AddDataSet(new G4NeutronCaptureXS());
  
  // Model
  theCaptureProcess->RegisterMe(new G4ParticleHPCapture());

  // // (re) create process: nCapture
  // //
  // G4NeutronCaptureProcess* process3 = new G4NeutronCaptureProcess();
  // pManager->AddDiscreteProcess(process3);
  // //
  // // cross section data set
  // G4ParticleHPCaptureData* dataSet3 = new G4ParticleHPCaptureData();
  // process3->AddDataSet(dataSet3);
  // //
  // // models
  // G4ParticleHPCapture* model3 = new G4ParticleHPCapture();
  // process3->RegisterMe(model3);


  //========================================================================//
  // NEUTRON FISSION
  //========================================================================//

  // Process
  G4NeutronFissionProcess* theFissionProcess = new G4NeutronFissionProcess();
  pManager->AddDiscreteProcess(theFissionProcess);
  
  // Cross section data set
  theFissionProcess->AddDataSet(new G4ParticleHPFissionData());
  
  // Model
  theFissionProcess->RegisterMe(new G4ParticleHPFission());

  // // Process
  // G4NeutronFissionProcess* process4 = new G4NeutronFissionProcess();
  // pManager->AddDiscreteProcess(process4);
  
  // // Cross section data set
  // G4ParticleHPFissionData* dataSet4 = new G4ParticleHPFissionData();
  // process4->AddDataSet(dataSet4);
  
  // // Model
  // G4ParticleHPFission* model4 = new G4ParticleHPFission();
  // process4->RegisterMe(model4);
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