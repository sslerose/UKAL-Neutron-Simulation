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
/// \file PrimaryGeneratorAction.hh
/// \brief Definition of the PrimaryGeneratorAction class
//

#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class G4Event;
class SimLiT;
class PrimaryGeneratorMessenger;
class DetectorConstruction;

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
///   /neutronTOF/gun/useSimLiT [true/false]
///   /neutronTOF/gun/beamEnergy [keV]
///   /neutronTOF/gun/beamSigma [keV]
///   /neutronTOF/gun/targetMaterial [Li/LiF/Li2O/...]
///   /neutronTOF/gun/targetThickness [um]
///   /neutronTOF/gun/energy [MeV] (for simple gun mode)
///   /neutronTOF/gun/printParameters

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

  public:
    void GeneratePrimaries(G4Event*) override;

    void SetTimeExposure(G4double value) { fTimeExposure = value; }
    G4double GetTimeExposure() { return fTimeExposure; }
    
    //========================================================================//
    // Accessor methods
    //========================================================================//
    G4ParticleGun* GetParticleGun() { return fParticleGun; }
    
    // Get last generated neutron parameters (for analysis)
    G4double GetNeutronEnergy() const { return fNeutronEnergy; }
    G4double GetNeutronTheta() const { return fNeutronTheta; }

  private:
    //========================================================================//
    // Sync local SimLiT with shared configuration
    //========================================================================//
    void SyncWithConfig();

    //========================================================================//
    // Map material name to SimLiT composition enum
    //========================================================================//
    int GetSimLiTComposition(const G4String& materialName);

    G4ParticleGun* fParticleGun = nullptr;
    SimLiT* fSimLiTSource = nullptr;
    
    //========================================================================//
    // Cached configuration values for detecting changes
    //========================================================================//
    G4double fCachedBeamEnergy = -1.;
    G4double fCachedBeamSigma = -1.;
    G4String fCachedTargetMaterial = "";
    G4double fCachedTargetThickness = -1.;
    G4double fCachedGunEnergy = -1.;

    //========================================================================//
    // Detector geometry cache for focused generation
    //========================================================================//
    G4bool fCachedLimitToDetector = false;
    G4double fCachedDetectorAngle = 0.;      // radians
    G4double fCachedDetectorDistance = 0.;   // Geant4 internal units

    // Computed angle limits (updated when detector params change)
    G4double fThetaMin = 0.;                 // radians
    G4double fThetaMax = CLHEP::pi;          // radians
    G4double fThetaHalfAngle = 0.;           // theta_r/2 in radians
    G4double fDetectorRadius = 0.;           // Geant4 internal units

    //========================================================================//
    // Last generated neutron parameters (for analysis)
    //========================================================================//
    G4double fNeutronEnergy = 0.;  // keV (SimLiT units)
    G4double fNeutronTheta = 0.;   // radians

    G4double fTimeExposure = 0.;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
