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
  // Create UI directory for detector commands
  //========================================================================//
  G4bool broadcast = false;
  fDetDir = new G4UIdirectory("/gammaSpec/det/", broadcast);
  fDetDir->SetGuidance("Detector construction commands");

  //========================================================================//
  // Detector distance from origin
  //========================================================================//
  fDetectorDistanceCmd = new G4UIcmdWithADoubleAndUnit("/gammaSpec/det/setDetectorDistance", this);
  fDetectorDistanceCmd->SetGuidance("Set distance from origin (neutron source) to detector face");
  fDetectorDistanceCmd->SetParameterName("DetectorDistance", false);
  fDetectorDistanceCmd->SetRange("DetectorDistance > 0.");
  fDetectorDistanceCmd->SetUnitCategory("Length");
  fDetectorDistanceCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
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
// Apply new parameter values based on user commands
//------------------------------------------------------------------------//
void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fDetectorDistanceCmd) {
    fDetector->SetDetectorDistance(fDetectorDistanceCmd->GetNewDoubleValue(newValue));
  }

  if (command == fWorldMaterialCmd) {
    fDetector->SetWorldMaterial(newValue);
  }

  if (command == fPrintCmd) {
    fDetector->PrintParameters();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......