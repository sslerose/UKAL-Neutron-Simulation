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
/// \file PrimaryGeneratorAction.hh
/// \brief Definition of the PrimaryGeneratorAction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4Event;
class SimLiT;
class PrimaryGeneratorMessenger;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

/// Primary generator action class
///
/// Generates primary neutrons using either:
/// 1. Simple particle gun (for testing)
/// 2. SimLiT source model (realistic 7Li(p,n)7Be spectrum)
///
/// The SimLiT model provides physically accurate neutron energy and
/// angular distributions from the Li(p,n) reaction used in the
/// experimental setup described in the Feinberg thesis.
///
/// UI Commands (via PrimaryGeneratorMessenger):
///   /neutrondet/gun/useSimLiT [true/false]
///   /neutrondet/gun/beamEnergy [keV]
///   /neutrondet/gun/beamSigma [keV]
///   /neutrondet/gun/targetMaterial [Li/LiF/Li2O/...]
///   /neutrondet/gun/targetThickness [um]
///   /neutrondet/gun/energy [MeV] (for simple gun mode)
///   /neutrondet/gun/printParameters

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

  public:
    void GeneratePrimaries(G4Event*) override;
    
    //========================================================================//
    // Accessor methods
    //========================================================================//
    G4ParticleGun* GetParticleGun() { return fParticleGun; }
    
    //========================================================================//
    // SimLiT enable/disable
    //========================================================================//
    void SetUseSimLiT(G4bool use) { fUseSimLiT = use; }
    G4bool GetUseSimLiT() const { return fUseSimLiT; }
    
    //========================================================================//
    // SimLiT beam parameters (energy in keV)
    //========================================================================//
    void SetBeamEnergy(G4double energy);
    G4double GetBeamEnergy() const;
    
    void SetBeamSigma(G4double sigma);
    G4double GetBeamSigma() const;
    
    //========================================================================//
    // SimLiT target parameters
    //========================================================================//
    void SetTargetMaterial(const G4String& material);
    G4String GetTargetMaterial() const;
    
    void SetTargetThickness(G4double thickness);  // in micrometers
    G4double GetTargetThickness() const;
    
    //========================================================================//
    // Information methods
    //========================================================================//
    void PrintParameters() const;
    
    // Get last generated neutron parameters (for analysis)
    G4double GetNeutronEnergy() const { return fNeutronEnergy; }
    G4double GetNeutronTheta() const { return fNeutronTheta; }

  private:
    G4ParticleGun* fParticleGun = nullptr;
    SimLiT* fSimLiTSource = nullptr;
    PrimaryGeneratorMessenger* fMessenger = nullptr;
    
    //========================================================================//
    // Configuration flags
    //========================================================================//
    G4bool fUseSimLiT = true;      // Use SimLiT by default
    
    //========================================================================//
    // Last generated neutron parameters (for analysis)
    //========================================================================//
    G4double fNeutronEnergy = 0.;  // keV (SimLiT units)
    G4double fNeutronTheta = 0.;   // radians
    
    //========================================================================//
    // Target material name (for PrintParameters)
    //========================================================================//
    G4String fTargetMaterialName = "LiF";
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
