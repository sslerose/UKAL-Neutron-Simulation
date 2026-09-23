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
  // Create GammaSpec UI directory
  //========================================================================//

  fGammaSpecDir = new G4UIdirectory("/gammaSpec/");
  fGammaSpecDir->SetGuidance("Commands specific to gamma spectroscopy simulation");


  //========================================================================//
  // World material
  //========================================================================//

  fWorldMaterialCmd = new G4UIcmdWithAString("/gammaSpec/setWorldMaterial", this);
  fWorldMaterialCmd->SetGuidance("Set material of the world volume");
  fWorldMaterialCmd->SetGuidance("  Available Materials:");
  fWorldMaterialCmd->SetGuidance("    air    - G4_AIR");
  fWorldMaterialCmd->SetGuidance("    vacuum - G4_Galactic");
  fWorldMaterialCmd->SetParameterName("WorldMaterial", false);
  fWorldMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Print current parameters
  //========================================================================//

  fPrintCmd = new G4UIcmdWithoutParameter("/gammaSpec/printParameters", this);
  fPrintCmd->SetGuidance("Print current system configuration");
  fPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle, G4State_GeomClosed);


  //========================================================================//
  // Create detector UI directory
  //========================================================================//

  G4bool broadcast = false;
  fDetDir = new G4UIdirectory("/gammaSpec/det/", broadcast);
  fDetDir->SetGuidance("Detector construction commands");


  //========================================================================//
  // Detector distance from origin
  //========================================================================//

  fDetectorDistanceCmd = new G4UIcommand("/gammaSpec/det/setDetectorDistance", this);
  fDetectorDistanceCmd->SetGuidance("Set distance from origin to detector face");

  // Detector ID
  G4UIparameter* detIDPrm = new G4UIparameter("detectorID", 'i', false);
  detIDPrm->SetGuidance("detector ID (0 or 1)");
  detIDPrm->SetParameterRange("detectorID==0 || detectorID==1");
  fDetectorDistanceCmd->SetParameter(detIDPrm);

  // Distance
  G4UIparameter* distPrm = new G4UIparameter("distance", 'd', false);
  distPrm->SetGuidance("distance from origin to detector face");
  distPrm->SetParameterRange("distance > 0.");
  fDetectorDistanceCmd->SetParameter(distPrm);

  // Unit of distance
  G4UIparameter* unitPrm = new G4UIparameter("unit", 's', false);
  unitPrm->SetGuidance("unit of distance");
  G4String unitList = G4UIcommand::UnitsList(G4UIcommand::CategoryOf("cm"));
  unitPrm->SetParameterCandidates(unitList);
  fDetectorDistanceCmd->SetParameter(unitPrm);

  fDetectorDistanceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Detector angle around y-axis
  //========================================================================//

  fDetectorAngleCmd = new G4UIcommand("/gammaSpec/det/setDetectorAngle", this);
  fDetectorAngleCmd->SetGuidance("Set angle of detector around y-axis (neutron source)");

  // Detector ID
  detIDPrm = new G4UIparameter("detectorID", 'i', false);
  detIDPrm->SetGuidance("detector ID (0 or 1)");
  detIDPrm->SetParameterRange("detectorID==0 || detectorID==1");
  fDetectorAngleCmd->SetParameter(detIDPrm);

  // Angle
  G4UIparameter* anglePrm = new G4UIparameter("angle", 'd', false);
  anglePrm->SetGuidance("angle of detector around y-axis");
  anglePrm->SetParameterRange("angle >= 0. && angle <= 360");
  fDetectorAngleCmd->SetParameter(anglePrm);

  // Unit of angle
  unitPrm = new G4UIparameter("unit", 's', false);
  unitPrm->SetGuidance("unit of angle");
  unitList = G4UIcommand::UnitsList(G4UIcommand::CategoryOf("deg"));
  unitPrm->SetParameterCandidates(unitList);
  fDetectorAngleCmd->SetParameter(unitPrm);

  fDetectorAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Create absorber UI directory
  //========================================================================//

  fAbsorDir = new G4UIdirectory("/gammaSpec/abs/", broadcast);
  fAbsorDir->SetGuidance("Absorber construction commands");


  //========================================================================//
  // Absorber material
  //========================================================================//

  fAbsorberMaterialCmd = new G4UIcommand("/gammaSpec/abs/setAbsorberMaterial", this);
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
  unitPrm = new G4UIparameter("unit", 's', false);
  unitPrm->SetGuidance("unit of density");
  unitList = G4UIcommand::UnitsList(G4UIcommand::CategoryOf("g/cm3"));
  unitPrm->SetParameterCandidates(unitList);
  fAbsorberMaterialCmd->SetParameter(unitPrm);

  fAbsorberMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Absorber thickness
  //========================================================================//
  fAbsorberThicknessCmd = new G4UIcmdWithADoubleAndUnit("/gammaSpec/abs/setAbsorberThickness", this);
  fAbsorberThicknessCmd->SetGuidance("Set thickness of the absorber in micrometers.");
  fAbsorberThicknessCmd->SetParameterName("thickness", false);
  fAbsorberThicknessCmd->SetRange("thickness > 0");
  fAbsorberThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Absorber radius
  //========================================================================//
  fAbsorberRadiusCmd = new G4UIcmdWithADoubleAndUnit("/gammaSpec/abs/setAbsorberRadius", this);
  fAbsorberRadiusCmd->SetGuidance("Set radius of the absorber in micrometers.");
  fAbsorberRadiusCmd->SetParameterName("radius", false);
  fAbsorberRadiusCmd->SetRange("radius > 0");
  fAbsorberRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Initial spanning angle of absorber
  //========================================================================//
  fSpanningStartAngleCmd = new G4UIcmdWithADoubleAndUnit("/gammaSpec/abs/setAbsorberSpanningStartAngle", this);
  fSpanningStartAngleCmd->SetGuidance("Set starting spanning angle of absorber");
  fSpanningStartAngleCmd->SetParameterName("spanningStartAngle", false);
  fSpanningStartAngleCmd->SetUnitCategory("Angle");
  fSpanningStartAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);


  //========================================================================//
  // Final spanning angle of absorber
  //========================================================================//
  fSpanningEndAngleCmd = new G4UIcmdWithADoubleAndUnit("/gammaSpec/abs/setAbsorberSpanningEndAngle", this);
  fSpanningEndAngleCmd->SetGuidance("Set ending spanning angle of absorber");
  fSpanningEndAngleCmd->SetParameterName("spanningEndAngle", false);
  fSpanningEndAngleCmd->SetUnitCategory("Angle");
  fSpanningEndAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::~DetectorMessenger()
{
  delete fGammaSpecDir;
  delete fDetDir;
  delete fDetectorDistanceCmd;
  delete fWorldMaterialCmd;
  delete fPrintCmd;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Apply new parameter values based on user command
//------------------------------------------------------------------------//
void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  // World material
  if (command == fWorldMaterialCmd) {
    fDetector->SetWorldMaterial(newValue);
  }

  // Print current parameters
  if (command == fPrintCmd) {
    fDetector->PrintParameters();
  }

  // Detector distance
  if (command == fDetectorDistanceCmd) {
    G4int id;
    G4double distance;
    G4String unt;
    std::istringstream is(newValue);
    is >> id >> distance >> unt;
    distance *= G4UIcommand::ValueOf(unt);
    fDetector->SetDetectorDistance(id, distance);
  }

  // Detector angle
  if (command == fDetectorAngleCmd) {
    G4int id;
    G4double angle;
    G4String unt;
    std::istringstream is(newValue);
    is >> id >> angle >> unt;
    angle *= G4UIcommand::ValueOf(unt);
    fDetector->SetDetectorAngle(id, angle);
  }

  // Absorber material
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

  // Absorber thickness
  if (command == fAbsorberThicknessCmd) {
    fDetector->SetAbsorberThickness(fAbsorberThicknessCmd->GetNewDoubleValue(newValue));
  }

  // Absorber radius
  if (command == fAbsorberRadiusCmd) {
    fDetector->SetAbsorberRadius(fAbsorberRadiusCmd->GetNewDoubleValue(newValue));
  }

  // Absorber initial spanning angle
  if (command == fSpanningStartAngleCmd) {
    fDetector->SetSpanningStartAngle(fSpanningStartAngleCmd->GetNewDoubleValue(newValue));
  }

  // Absorber final spanning angle
  if (command == fSpanningEndAngleCmd) {
    fDetector->SetSpanningEndAngle(fSpanningEndAngleCmd->GetNewDoubleValue(newValue));
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......