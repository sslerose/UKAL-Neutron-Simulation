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
/// \file DetectorMessenger.cc
/// \brief Implementation of the DetectorMessenger class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIdirectory.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::DetectorMessenger(DetectorConstruction* Det) : fDetector(Det)
{
  fNeutronDetDir = new G4UIdirectory("/neutrondet/");
  fNeutronDetDir->SetGuidance("Commands specific to neutron detection simulation");

  G4bool broadcast = false;
  fDetDir = new G4UIdirectory("/neutrondet/det/", broadcast);
  fDetDir->SetGuidance("Detector construction commands");

  // Detector thickness
  fDetectorThicknessCmd = new G4UIcmdWithADoubleAndUnit("/neutrondet/det/setDetectorThickness", this);
  fDetectorThicknessCmd->SetGuidance("Set thickness of Li-6 glass detector");
  fDetectorThicknessCmd->SetParameterName("DetectorThickness", false);
  fDetectorThicknessCmd->SetRange("DetectorThickness>0.");
  fDetectorThicknessCmd->SetUnitCategory("Length");
  fDetectorThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Detector radius
  fDetectorRadiusCmd = new G4UIcmdWithADoubleAndUnit("/neutrondet/det/setDetectorRadius", this);
  fDetectorRadiusCmd->SetGuidance("Set radius of cylindrical Li-6 glass detector");
  fDetectorRadiusCmd->SetParameterName("DetectorRadius", false);
  fDetectorRadiusCmd->SetRange("DetectorRadius>0.");
  fDetectorRadiusCmd->SetUnitCategory("Length");
  fDetectorRadiusCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Detector distance from origin
  fDetectorDistanceCmd = new G4UIcmdWithADoubleAndUnit("/neutrondet/det/setDetectorDistance", this);
  fDetectorDistanceCmd->SetGuidance("Set distance from origin (neutron source) to detector face");
  fDetectorDistanceCmd->SetParameterName("DetectorDistance", false);
  fDetectorDistanceCmd->SetRange("DetectorDistance>0.");
  fDetectorDistanceCmd->SetUnitCategory("Length");
  fDetectorDistanceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Initial spanning angle of detector
  fDetectorSpanningStartAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutrondet/det/setSpanningStartAngle", this);
  fDetectorSpanningStartAngleCmd->SetGuidance("Set starting spanning angle of detector");
  fDetectorSpanningStartAngleCmd->SetParameterName("SpanningStartAngle", false);
  fDetectorSpanningStartAngleCmd->SetUnitCategory("Angle");
  fDetectorSpanningStartAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Final spanning angle of detector
  fDetectorSpanningEndAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutrondet/det/setSpanningEndAngle", this);
  fDetectorSpanningEndAngleCmd->SetGuidance("Set ending spanning angle of detector");
  fDetectorSpanningEndAngleCmd->SetParameterName("SpanningEndAngle", false);
  fDetectorSpanningEndAngleCmd->SetUnitCategory("Angle");
  fDetectorSpanningEndAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::~DetectorMessenger()
{
  delete fDetectorThicknessCmd;
  delete fDetectorRadiusCmd;
  delete fDetectorDistanceCmd;
  delete fDetectorSpanningStartAngleCmd;
  delete fDetectorSpanningEndAngleCmd;
  delete fDetDir;
  delete fNeutronDetDir;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fDetectorThicknessCmd) {
    fDetector->SetDetectorThickness(fDetectorThicknessCmd->GetNewDoubleValue(newValue));
  }

  if (command == fDetectorRadiusCmd) {
    fDetector->SetDetectorRadius(fDetectorRadiusCmd->GetNewDoubleValue(newValue));
  }

  if (command == fDetectorDistanceCmd) {
    fDetector->SetDetectorDistance(fDetectorDistanceCmd->GetNewDoubleValue(newValue));
  }

  if (command == fDetectorSpanningStartAngleCmd) {
    fDetector->SetSpanningStartAngle(fDetectorSpanningStartAngleCmd->GetNewDoubleValue(newValue));
  }

  if (command == fDetectorSpanningEndAngleCmd) {
    fDetector->SetSpanningEndAngle(fDetectorSpanningEndAngleCmd->GetNewDoubleValue(newValue));
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......