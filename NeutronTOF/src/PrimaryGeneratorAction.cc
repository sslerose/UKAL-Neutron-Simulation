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
/// \file PrimaryGeneratorAction.cc
/// \brief Implementation of the PrimaryGeneratorAction class
//

#include "PrimaryGeneratorAction.hh"
#include "PrimaryGeneratorMessenger.hh"
#include "PrimaryGeneratorConfig.hh"
#include "SimLiT.hh"

#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "Randomize.hh"

#include <cmath>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  //========================================================================//
  // Initialize particle gun for neutrons
  //========================================================================//

  // Create particle gun with one particle per event
  G4int nParticle = 1;
  fParticleGun = new G4ParticleGun(nParticle);
  
  // Set particle type to neutron
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* particle = particleTable->FindParticle("neutron");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticlePosition(G4ThreeVector(0., 0., 0.));

  // Get initial configuration from shared config
  PrimaryGeneratorConfig* config = PrimaryGeneratorConfig::Instance();
  

  //========================================================================//
  // Default particle gun settings (used when SimLiT is disabled)
  //========================================================================//

  fParticleGun->SetParticleEnergy(config->GetGunEnergy());
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  

  //========================================================================//
  // Initialize SimLiT neutron source with config values
  //========================================================================//

  fSimLiTSource = new SimLiT(config->GetBeamEnergy(), config->GetBeamSigma());
  
  // Set target material
  int composition = GetSimLiTComposition(config->GetTargetMaterial());
  if (composition >= 0) {
    fSimLiTSource->SetComposition(composition);
  }
  
  // Set target thickness (convert um to cm for SimLiT)
  fSimLiTSource->SetTargetThickness(config->GetTargetThickness() * SimLiT::um);
  
  
  //========================================================================//
  // Cache initial values for change detection
  //========================================================================//
  fCachedBeamEnergy = config->GetBeamEnergy();
  fCachedBeamSigma = config->GetBeamSigma();
  fCachedTargetMaterial = config->GetTargetMaterial();
  fCachedTargetThickness = config->GetTargetThickness();
  fCachedGunEnergy = config->GetGunEnergy();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fSimLiTSource;
  // delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Sync local primary generator parameters with shared configuration
//------------------------------------------------------------------------//
void PrimaryGeneratorAction::SyncWithConfig()
{
  //========================================================================//
  // Sync local SimLiT with shared configuration
  // Only update parameters that have changed to minimize overhead
  //========================================================================//
  
  PrimaryGeneratorConfig* config = PrimaryGeneratorConfig::Instance();
  
  // Check and update SimLiT beam energy
  G4double beamEnergy = config->GetBeamEnergy();
  if (beamEnergy != fCachedBeamEnergy) {
    fSimLiTSource->SetBeamEnergy(beamEnergy);
    fCachedBeamEnergy = beamEnergy;
  }
  
  // Check and update SimLiT beam sigma
  G4double beamSigma = config->GetBeamSigma();
  if (beamSigma != fCachedBeamSigma) {
    fSimLiTSource->SetBeamSigma(beamSigma);
    fCachedBeamSigma = beamSigma;
  }
  
  // Check and update SimLiT target material
  G4String targetMaterial = config->GetTargetMaterial();
  if (targetMaterial != fCachedTargetMaterial) {
    int composition = GetSimLiTComposition(targetMaterial);
    if (composition >= 0) {
      fSimLiTSource->SetComposition(composition);
      fCachedTargetMaterial = targetMaterial;
    }
    else {
      G4cerr << "PrimaryGeneratorAction::SyncWithConfig:" << G4endl;
      G4cerr << "Unknown target material: '" << targetMaterial << "'" << G4endl;
      G4cerr << "Keeping previous material: '" << fCachedTargetMaterial << "'" << G4endl;
    }
  }
  
  // Check and update SimLiT target thickness
  G4double targetThickness = config->GetTargetThickness();
  if (targetThickness != fCachedTargetThickness) {
    fSimLiTSource->SetTargetThickness(targetThickness * SimLiT::um);
    fCachedTargetThickness = targetThickness;
  }
  
  // Check and update gun energy (simple mode)
  G4double gunEnergy = config->GetGunEnergy();
  if (gunEnergy != fCachedGunEnergy) {
    fParticleGun->SetParticleEnergy(gunEnergy);
    fCachedGunEnergy = gunEnergy;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Map material name to SimLiT composition enum
//------------------------------------------------------------------------//
int PrimaryGeneratorAction::GetSimLiTComposition(const G4String& materialName)
{  
  if (materialName == "Li") return SimLiT::Li;
  if (materialName == "LiF") return SimLiT::LiF;
  if (materialName == "Li2O") return SimLiT::Li2O;
  if (materialName == "Li3N") return SimLiT::Li3N;
  if (materialName == "LiOH") return SimLiT::LiOH;
  if (materialName == "LiH") return SimLiT::LiH;
  
  return -1;  // Unknown material
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Generate particle(s) for each event
//------------------------------------------------------------------------//
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  //========================================================================//
  // Sync with shared config before generating
  // This picks up any parameter changes from UI commands
  //========================================================================//
  SyncWithConfig();
  
  // Get current mode from shared config
  G4bool useSimLiT = PrimaryGeneratorConfig::Instance()->GetUseSimLiT();
  
  if (useSimLiT && fSimLiTSource) {
    //====================================================================//
    // SimLiT mode: Generate neutron from 7Li(p,n)7Be reaction
    //====================================================================//
    
    double energy_keV;  // neutron energy in keV
    double theta_rad;   // neutron angle in radians (from beam axis)
    
    // Generate neutron energy and angle from SimLiT
    // GenerateNeutron stores energy and theta to reference parameters (energy_keV, theta_rad)
    fSimLiTSource->GenerateNeutron(energy_keV, theta_rad);
    
    // Store for analysis access
    fNeutronEnergy = energy_keV;
    fNeutronTheta = theta_rad;
    
    // Convert SimLiT energy (keV) to Geant4 internal units
    G4double energy = energy_keV * keV;
    
    // Generate random azimuthal angle (phi)
    G4double phi = twopi * G4UniformRand();
    
    // Calculate momentum direction from theta and phi
    G4double sinTheta = std::sin(theta_rad);
    G4double cosTheta = std::cos(theta_rad);
    
    G4ThreeVector direction(
      sinTheta * std::cos(phi),
      sinTheta * std::sin(phi),
      cosTheta
    );
    
    // Configure particle gun and generate vertex
    fParticleGun->SetParticleEnergy(energy);
    fParticleGun->SetParticleMomentumDirection(direction);
    fParticleGun->GeneratePrimaryVertex(anEvent);
  }
  else {
    //====================================================================//
    // Simple mode: Use particle gun with configured energy
    //====================================================================//
    
    // Simple mode: all neutrons along z-axis for testing
    G4ThreeVector direction(0, 0, 1);
    
    fParticleGun->SetParticleMomentumDirection(direction);
    fParticleGun->GeneratePrimaryVertex(anEvent);
    
    // Store for consistency
    fNeutronEnergy = fParticleGun->GetParticleEnergy() / keV;
    fNeutronTheta = 0.;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......