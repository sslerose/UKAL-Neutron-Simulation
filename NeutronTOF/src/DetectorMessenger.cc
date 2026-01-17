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
#include "G4UIdirectory.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::DetectorMessenger(DetectorConstruction* Det) : fDetector(Det)
{
  fNeutronDetDir = new G4UIdirectory("/neutronTOF/");
  fNeutronDetDir->SetGuidance("Commands specific to neutron detection simulation");

  G4bool broadcast = false;
  fDetDir = new G4UIdirectory("/neutronTOF/det/", broadcast);
  fDetDir->SetGuidance("Detector construction commands");

  // Detector distance from origin
  fDetectorDistanceCmd = new G4UIcmdWithADoubleAndUnit("/neutronTOF/det/setDetectorDistance", this);
  fDetectorDistanceCmd->SetGuidance("Set distance from origin (neutron source) to detector face");
  fDetectorDistanceCmd->SetParameterName("DetectorDistance", false);
  fDetectorDistanceCmd->SetRange("DetectorDistance>0.");
  fDetectorDistanceCmd->SetUnitCategory("Length");
  fDetectorDistanceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Detector angle around y-axis
  fDetectorAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutronTOF/det/setDetectorAngle", this);
  fDetectorAngleCmd->SetGuidance("Set angle of detector around y-axis (neutron source)");
  fDetectorAngleCmd->SetParameterName("DetectorAngle", false);
  fDetectorAngleCmd->SetUnitCategory("Angle");
  fDetectorAngleCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Initial spanning angle of detector
  fDetectorSpanningStartAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutronTOF/det/setSpanningStartAngle", this);
  fDetectorSpanningStartAngleCmd->SetGuidance("Set starting spanning angle of detector");
  fDetectorSpanningStartAngleCmd->SetParameterName("SpanningStartAngle", false);
  fDetectorSpanningStartAngleCmd->SetUnitCategory("Angle");
  fDetectorSpanningStartAngleCmd->AvailableForStates(G4State_PreInit);

  // Final spanning angle of detector
  fDetectorSpanningEndAngleCmd = new G4UIcmdWithADoubleAndUnit("/neutronTOF/det/setSpanningEndAngle", this);
  fDetectorSpanningEndAngleCmd->SetGuidance("Set ending spanning angle of detector");
  fDetectorSpanningEndAngleCmd->SetParameterName("SpanningEndAngle", false);
  fDetectorSpanningEndAngleCmd->SetUnitCategory("Angle");
  fDetectorSpanningEndAngleCmd->AvailableForStates(G4State_PreInit);

  // Print current parameters
  fPrintCmd = new G4UIcmdWithoutParameter("/neutronTOF/det/printParameters", this);
  fPrintCmd->SetGuidance("Print current neutron source configuration.");
  fPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle, G4State_GeomClosed);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorMessenger::~DetectorMessenger()
{
  delete fDetectorDistanceCmd;
  delete fDetectorAngleCmd;
  delete fDetectorSpanningStartAngleCmd;
  delete fDetectorSpanningEndAngleCmd;
  delete fDetDir;
  delete fNeutronDetDir;
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

  if (command == fDetectorSpanningStartAngleCmd) {
    fDetector->SetSpanningStartAngle(fDetectorSpanningStartAngleCmd->GetNewDoubleValue(newValue));
  }

  if (command == fDetectorSpanningEndAngleCmd) {
    fDetector->SetSpanningEndAngle(fDetectorSpanningEndAngleCmd->GetNewDoubleValue(newValue));
  }

  if (command == fPrintCmd) {
    fDetector->PrintParameters();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......