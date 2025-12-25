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
/// \file PrimaryGeneratorMessenger.hh
/// \brief Definition of the PrimaryGeneratorMessenger class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef PrimaryGeneratorMessenger_h
#define PrimaryGeneratorMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class PrimaryGeneratorAction;
class G4UIdirectory;
class G4UIcmdWithABool;
class G4UIcmdWithADouble;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithAString;
class G4UIcmdWithoutParameter;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

/// Messenger class for PrimaryGeneratorAction
///
/// Provides UI commands to control the neutron source:
///
/// /neutrondet/gun/useSimLiT [true/false]
///   - Enable/disable SimLiT source (vs simple particle gun)
///
/// /neutrondet/gun/beamEnergy [value] keV
///   - Set proton beam energy for Li(p,n) reaction
///
/// /neutrondet/gun/beamSigma [value] keV
///   - Set proton beam energy spread (1-sigma)
///
/// /neutrondet/gun/targetMaterial [Li/LiF/Li2O/Li3N/LiOH/LiH]
///   - Set lithium target composition
///
/// /neutrondet/gun/targetThickness [value] um
///   - Set lithium target thickness
///
/// /neutrondet/gun/printParameters
///   - Print current source configuration

class PrimaryGeneratorMessenger : public G4UImessenger
{
  public:
    PrimaryGeneratorMessenger(PrimaryGeneratorAction*);
    ~PrimaryGeneratorMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

  private:
    PrimaryGeneratorAction* fPrimaryGenerator = nullptr;

    //========================================================================//
    // UI directories
    //========================================================================//
    G4UIdirectory* fGunDir = nullptr;

    //========================================================================//
    // UI commands
    //========================================================================//
    G4UIcmdWithABool*          fUseSimLiTCmd = nullptr;
    G4UIcmdWithADouble*        fBeamEnergyCmd = nullptr;
    G4UIcmdWithADouble*        fBeamSigmaCmd = nullptr;
    G4UIcmdWithAString*        fTargetMaterialCmd = nullptr;
    G4UIcmdWithADouble*        fTargetThicknessCmd = nullptr;
    G4UIcmdWithoutParameter*   fPrintCmd = nullptr;
    
    //========================================================================//
    // Simple gun energy command (when SimLiT disabled)
    //========================================================================//
    G4UIcmdWithADoubleAndUnit* fGunEnergyCmd = nullptr;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
