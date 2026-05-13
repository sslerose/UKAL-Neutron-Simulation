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
/// \file DetectorMessenger.cc
/// \brief Implementation of the DetectorMessenger class
//

#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Create detector commands and directories
//------------------------------------------------------------------------//
DetectorMessenger::DetectorMessenger(DetectorConstruction* Det) : fDetector(Det)
{
  //========================================================================//
  // Create UI directory for all simulation commands
  //========================================================================//
  fNeutronDetDir = new G4UIdirectory("/neutronAct/");
  fNeutronDetDir->SetGuidance("Commands specific to neutron activation simulation");

  //========================================================================//
  // World material
  //========================================================================//
  fWorldMaterialCmd = new G4UIcmdWithAString("/neutronAct/setWorldMaterial", this);
  fWorldMaterialCmd->SetGuidance("Set material of the world volume");
  fWorldMaterialCmd->SetGuidance("  Available Materials:");
  fWorldMaterialCmd->SetGuidance("    air    - G4_AIR");
  fWorldMaterialCmd->SetGuidance("    vacuum - G4_Galactic");
  fWorldMaterialCmd->SetParameterName("WorldMaterial", false);
  fWorldMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Print current parameters
  //========================================================================//
  fPrintCmd = new G4UIcmdWithoutParameter("/neutronAct/printParameters", this);
  fPrintCmd->SetGuidance("Print current system configuration");
  fPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle, G4State_GeomClosed);

  //========================================================================//
  // Create UI directory for detector commands
  //========================================================================//
  G4bool broadcast = false;
  fDetDir = new G4UIdirectory("/neutronAct/det/", broadcast);
  fDetDir->SetGuidance("Detector construction commands");

  //========================================================================//
  // Create UI directory for absorber commands
  //========================================================================//
  fAbsorDir = new G4UIdirectory("/neutronAct/abs/", broadcast);
  fAbsorDir->SetGuidance("Absorber construction commands");

  //========================================================================//
  // Detector distance from origin
  //========================================================================//
  fDetectorDistanceCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/det/setDetectorDistance", this);
  fDetectorDistanceCmd->SetGuidance("Set distance from origin (neutron source) to detector face");
  fDetectorDistanceCmd->SetParameterName("DetectorDistance", false);
  fDetectorDistanceCmd->SetRange("DetectorDistance > 0.");
  fDetectorDistanceCmd->SetUnitCategory("Length");
  fDetectorDistanceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Detector angle around y-axis
  //========================================================================//
  fDetectorAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/det/setDetectorAngle", this);
  fDetectorAngleCmd->SetGuidance("Set angle of detector around y-axis (neutron source)");
  fDetectorAngleCmd->SetParameterName("DetectorAngle", false);
  fDetectorAngleCmd->SetUnitCategory("Angle");
  fDetectorAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Absorber material
  //========================================================================//
  fAbsorberMaterialCmd = new G4UIcommand("/neutronAct/abs/setAbsorberMaterial", this);
  fAbsorberMaterialCmd->SetGuidance("Build and select a material with single isotope");
  fAbsorberMaterialCmd->SetGuidance("  symbol of isotope, Z, A, density of material, unit of density");
  
  // Isotope symbol
  G4UIparameter* symbPrm = new G4UIparameter("isotope", 's', false);
  symbPrm->SetGuidance("isotope symbol");
  fAbsorberMaterialCmd->SetParameter(symbPrm);
  
  // Isotope atomic number
  G4UIparameter* ZPrm = new G4UIparameter("Z", 'i', false);
  ZPrm->SetGuidance("Z (atomic number)");
  ZPrm->SetParameterRange("Z>0");
  fAbsorberMaterialCmd->SetParameter(ZPrm);
  
  // Isotope mass number
  G4UIparameter* APrm = new G4UIparameter("A", 'i', false);
  APrm->SetGuidance("A (mass number)");
  APrm->SetParameterRange("A>0");
  fAbsorberMaterialCmd->SetParameter(APrm);
  
  // Isotope density
  G4UIparameter* densityPrm = new G4UIparameter("density", 'd', false);
  densityPrm->SetGuidance("material density");
  densityPrm->SetParameterRange("density>0.");
  fAbsorberMaterialCmd->SetParameter(densityPrm);
  
  // Unit of density
  G4UIparameter* unitPrm = new G4UIparameter("unit", 's', false);
  unitPrm->SetGuidance("unit of density");
  G4String unitList = G4UIcommand::UnitsList(G4UIcommand::CategoryOf("g/cm3"));
  unitPrm->SetParameterCandidates(unitList);
  fAbsorberMaterialCmd->SetParameter(unitPrm);
  
  fAbsorberMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Absorber distance
  //========================================================================//
  fAbsorberDistanceCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/abs/setAbsorberDistance", this);
  fAbsorberDistanceCmd->SetGuidance("Set distance from origin (neutron source) to absorber face");
  fAbsorberDistanceCmd->SetParameterName("distance", false);
  fAbsorberDistanceCmd->SetRange("distance > 0");
  fAbsorberDistanceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Absorber thickness
  //========================================================================//
  fAbsorberThicknessCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/abs/setAbsorberThickness", this);
  fAbsorberThicknessCmd->SetGuidance("Set thickness of the absorber in micrometers.");
  fAbsorberThicknessCmd->SetParameterName("thickness", false);
  fAbsorberThicknessCmd->SetRange("thickness > 0");
  fAbsorberThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Absorber radius
  //========================================================================//
  fAbsorberRadiusCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/abs/setAbsorberRadius", this);
  fAbsorberRadiusCmd->SetGuidance("Set radius of the absorber in micrometers.");
  fAbsorberRadiusCmd->SetParameterName("radius", false);
  fAbsorberRadiusCmd->SetRange("radius > 0");
  fAbsorberRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Initial spanning angle of absorber
  //========================================================================//
  fSpanningStartAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/abs/setAbsorberSpanningStartAngle", this);
  fSpanningStartAngleCmd->SetGuidance("Set starting spanning angle of absorber");
  fSpanningStartAngleCmd->SetParameterName("SpanningStartAngle", false);
  fSpanningStartAngleCmd->SetUnitCategory("Angle");
  fSpanningStartAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Final spanning angle of absorber
  //========================================================================//
  fSpanningEndAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutronAct/abs/setAbsorberSpanningEndAngle", this);
  fSpanningEndAngleCmd->SetGuidance("Set ending spanning angle of absorber");
  fSpanningEndAngleCmd->SetParameterName("SpanningEndAngle", false);
  fSpanningEndAngleCmd->SetUnitCategory("Angle");
  fSpanningEndAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::~DetectorMessenger()
{
  delete fNeutronDetDir;
  delete fDetDir;
  delete fAbsorDir;
  delete fDetectorDistanceCmd;
  delete fDetectorAngleCmd;
  delete fSpanningStartAngleCmd;
  delete fSpanningEndAngleCmd;
  delete fWorldMaterialCmd;
  delete fAbsorberMaterialCmd;
  delete fAbsorberThicknessCmd;
  delete fAbsorberRadiusCmd;
  delete fAbsorberDistanceCmd;
  delete fPrintCmd;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Apply new parameter values based on user commands
//------------------------------------------------------------------------//
void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fDetectorDistanceCmd) {
    fDetector->SetDetectorDistance(fDetectorDistanceCmd->GetNewDoubleValue(newValue));
  }

  if (command == fDetectorAngleCmd) {
    fDetector->SetDetectorAngle(fDetectorAngleCmd->GetNewDoubleValue(newValue));
  }

  if (command == fSpanningStartAngleCmd) {
    fDetector->SetSpanningStartAngle(fSpanningStartAngleCmd->GetNewDoubleValue(newValue));
  }

  if (command == fSpanningEndAngleCmd) {
    fDetector->SetSpanningEndAngle(fSpanningEndAngleCmd->GetNewDoubleValue(newValue));
  }

  if (command == fWorldMaterialCmd) {
    fDetector->SetWorldMaterial(newValue);
  }

  if (command == fAbsorberMaterialCmd) {
    G4int Z;
    G4int A;
    G4double dens;
    G4String name, unt;
    std::istringstream is(newValue);
    is >> name >> Z >> A >> dens >> unt;
    dens *= G4UIcommand::ValueOf(unt);
    fDetector->MaterialWithSingleIsotope(name, name, dens, Z, A);
    fDetector->SetAbsorberMaterial(name);
  }

  if (command == fAbsorberThicknessCmd) {
    fDetector->SetAbsorberThickness(fAbsorberThicknessCmd->GetNewDoubleValue(newValue));
  }

  if (command == fAbsorberRadiusCmd) {
    fDetector->SetAbsorberRadius(fAbsorberRadiusCmd->GetNewDoubleValue(newValue));
  }

  if (command == fAbsorberDistanceCmd) {
    fDetector->SetAbsorberDistance(fAbsorberDistanceCmd->GetNewDoubleValue(newValue));
  }

  if (command == fPrintCmd) {
    fDetector->PrintParameters();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......