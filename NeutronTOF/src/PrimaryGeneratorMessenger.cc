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
/// \file PrimaryGeneratorMessenger.cc
/// \brief Implementation of the PrimaryGeneratorMessenger class
//

#include "PrimaryGeneratorMessenger.hh"
#include "PrimaryGeneratorConfig.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIparameter.hh"
#include "G4UnitsTable.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorMessenger::PrimaryGeneratorMessenger()
{
  G4cout << "\n=== PrimaryGeneratorMessenger Constructor ===" << G4endl;

  //========================================================================//
  // Create UI directory for gun commands
  // Uses same /neutronTOF/ base as DetectorMessenger for consistency
  //========================================================================//
  fGunDir = new G4UIdirectory("/neutronTOF/gun/");
  fGunDir->SetGuidance("Primary generator (neutron source) control.");
  fGunDir->SetGuidance("Controls SimLiT Li(p,n) source or simple particle gun.");

  //========================================================================//
  // Enable/disable SimLiT source
  //========================================================================//
  fUseSimLiTCmd = new G4UIcmdWithABool("/neutronTOF/gun/useSimLiT", this);
  fUseSimLiTCmd->SetGuidance("Enable or disable SimLiT neutron source.");
  fUseSimLiTCmd->SetGuidance("  true  = Use SimLiT 7Li(p,n)7Be source (realistic spectrum)");
  fUseSimLiTCmd->SetGuidance("  false = Use simple particle gun (for testing)");
  fUseSimLiTCmd->SetParameterName("useSimLiT", false);
  fUseSimLiTCmd->SetDefaultValue(true);
  fUseSimLiTCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Proton beam energy (keV)
  //========================================================================//
  fBeamEnergyCmd = new G4UIcmdWithADouble("/neutronTOF/gun/beamEnergy", this);
  fBeamEnergyCmd->SetGuidance("Set proton beam energy for SimLiT source (in keV).");
  fBeamEnergyCmd->SetGuidance("  Threshold: 1880.4 keV");
  fBeamEnergyCmd->SetGuidance("  Typical values: 1912-2500 keV");
  fBeamEnergyCmd->SetGuidance("  Reference neutron energies at 0 degrees:");
  fBeamEnergyCmd->SetGuidance("    1912 keV -> ~30 keV neutrons");
  fBeamEnergyCmd->SetGuidance("    1950 keV -> ~70 keV neutrons");
  fBeamEnergyCmd->SetGuidance("    2000 keV -> ~120 keV neutrons");
  fBeamEnergyCmd->SetGuidance("    2500 keV -> ~600 keV neutrons");
  fBeamEnergyCmd->SetParameterName("beamEnergy", false);
  fBeamEnergyCmd->SetRange("beamEnergy >= 1880.4");
  fBeamEnergyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Proton beam energy spread (keV)
  //========================================================================//
  fBeamSigmaCmd = new G4UIcmdWithADouble("/neutronTOF/gun/beamSigma", this);
  fBeamSigmaCmd->SetGuidance("Set proton beam energy spread (1-sigma) in keV.");
  fBeamSigmaCmd->SetGuidance("  Typical values: 5-50 keV");
  fBeamSigmaCmd->SetParameterName("beamSigma", false);
  fBeamSigmaCmd->SetRange("beamSigma > 0");
  fBeamSigmaCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Target material composition
  //========================================================================//
  fTargetMaterialCmd = new G4UIcmdWithAString("/neutronTOF/gun/targetMaterial", this);
  fTargetMaterialCmd->SetGuidance("Set lithium target composition for SimLiT.");
  fTargetMaterialCmd->SetGuidance("  Available materials:");
  fTargetMaterialCmd->SetGuidance("    Li   - Pure lithium");
  fTargetMaterialCmd->SetGuidance("    LiF  - Lithium fluoride");
  fTargetMaterialCmd->SetGuidance("    Li2O - Lithium oxide");
  fTargetMaterialCmd->SetGuidance("    Li3N - Lithium nitride");
  fTargetMaterialCmd->SetGuidance("    LiOH - Lithium hydroxide");
  fTargetMaterialCmd->SetGuidance("    LiH  - Lithium hydride");
  fTargetMaterialCmd->SetParameterName("material", false);
  fTargetMaterialCmd->SetCandidates("Li LiF Li2O Li3N LiOH LiH");
  fTargetMaterialCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Target thickness (micrometers)
  //========================================================================//
  fTargetThicknessCmd = new G4UIcmdWithADouble("/neutronTOF/gun/targetThickness", this);
  fTargetThicknessCmd->SetGuidance("Set lithium target thickness in micrometers.");
  fTargetThicknessCmd->SetGuidance("  Typical values: 5-50 um");
  fTargetThicknessCmd->SetParameterName("thickness", false);
  fTargetThicknessCmd->SetRange("thickness > 0");
  fTargetThicknessCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  //========================================================================//
  // Print current parameters
  //========================================================================//
  fPrintCmd = new G4UIcmdWithoutParameter("/neutronTOF/gun/printParameters", this);
  fPrintCmd->SetGuidance("Print current neutron source configuration.");
  fPrintCmd->AvailableForStates(G4State_PreInit, G4State_Idle, G4State_GeomClosed);
  // fPrintCmd->AvailableForStates(G4State_Idle, G4State_GeomClosed);

  //========================================================================//
  // Simple gun energy (when SimLiT disabled)
  //========================================================================//
  fGunEnergyCmd = new G4UIcmdWithADoubleAndUnit("/neutronTOF/gun/energy", this);
  fGunEnergyCmd->SetGuidance("Set neutron energy for simple particle gun mode.");
  fGunEnergyCmd->SetGuidance("  Only used when SimLiT is disabled.");
  fGunEnergyCmd->SetParameterName("energy", false);
  fGunEnergyCmd->SetRange("energy > 0");
  fGunEnergyCmd->SetUnitCategory("Energy");
  fGunEnergyCmd->SetDefaultUnit("MeV");
  fGunEnergyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorMessenger::~PrimaryGeneratorMessenger()
{
  delete fUseSimLiTCmd;
  delete fBeamEnergyCmd;
  delete fBeamSigmaCmd;
  delete fTargetMaterialCmd;
  delete fTargetThicknessCmd;
  delete fPrintCmd;
  delete fGunEnergyCmd;
  delete fGunDir;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  // Get the singleton config instance
  PrimaryGeneratorConfig* config = PrimaryGeneratorConfig::Instance();

  //========================================================================//
  // Enable/disable SimLiT
  //========================================================================//
  if (command == fUseSimLiTCmd) {
    config->SetUseSimLiT(fUseSimLiTCmd->GetNewBoolValue(newValue));
  }

  //========================================================================//
  // Beam energy
  //========================================================================//
  else if (command == fBeamEnergyCmd) {
    config->SetBeamEnergy(fBeamEnergyCmd->GetNewDoubleValue(newValue));
  }

  //========================================================================//
  // Beam sigma
  //========================================================================//
  else if (command == fBeamSigmaCmd) {
    config->SetBeamSigma(fBeamSigmaCmd->GetNewDoubleValue(newValue));
  }

  //========================================================================//
  // Target material
  //========================================================================//
  else if (command == fTargetMaterialCmd) {
    config->SetTargetMaterial(newValue);
  }

  //========================================================================//
  // Target thickness
  //========================================================================//
  else if (command == fTargetThicknessCmd) {
    config->SetTargetThickness(fTargetThicknessCmd->GetNewDoubleValue(newValue));
  }

  //========================================================================//
  // Print parameters - reads from shared config, works from any thread
  //========================================================================//
  else if (command == fPrintCmd) {
    config->PrintParameters();
  }

  //========================================================================//
  // Simple gun energy
  //========================================================================//
  else if (command == fGunEnergyCmd) {
    config->SetGunEnergy(fGunEnergyCmd->GetNewDoubleValue(newValue));
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......