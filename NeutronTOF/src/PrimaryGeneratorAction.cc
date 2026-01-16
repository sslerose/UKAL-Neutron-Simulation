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
  
  // //========================================================================//
  // // Default particle gun settings (used when SimLiT is disabled)
  // // These provide a simple test configuration
  // //========================================================================//
  // fParticleGun->SetParticleEnergy(50.0*keV);
  // fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  
  // //========================================================================//
  // // Initialize SimLiT neutron source
  // // Default: 1950 keV proton beam with 10 keV sigma
  // // This produces ~70 keV neutrons at 0 degrees
  // //========================================================================//
  // G4double beamEnergy = 1912.0;  // keV - proton beam energy
  // G4double beamSigma = 10.0;     // keV - energy spread (1-sigma)
  
  // fSimLiTSource = new SimLiT(beamEnergy, beamSigma);
  // fTargetMaterialName = "LiF";  // Default target
  
  // fUseSimLiT = false;  // Use SimLiT by default
  
  // //========================================================================//
  // // Create messenger for UI commands
  // //========================================================================//
  // fMessenger = new PrimaryGeneratorMessenger(this);
  
  // //========================================================================//
  // // Print initial configuration
  // //========================================================================//
  // PrintParameters();
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
    
    // Generate isotropic direction in forward hemisphere
    // G4double cosTheta = G4UniformRand();  // cos(theta) uniform in [0, 1]
    // G4double sinTheta = std::sqrt(1. - cosTheta*cosTheta);
    // G4double phi = twopi * G4UniformRand();
    
    // Simple mode: all neutrons along z-axis for testing
    G4ThreeVector direction(0, 0, 1);
    
    fParticleGun->SetParticleMomentumDirection(direction);
    fParticleGun->GeneratePrimaryVertex(anEvent);
    
    // Store for consistency
    fNeutronEnergy = fParticleGun->GetParticleEnergy() / keV;
    // fNeutronTheta = std::acos(cosTheta);
    fNeutronTheta = 0.;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
// {
//   //========================================================================//
//   // Generate primary neutron vertex
//   //========================================================================//
  
//   if (fUseSimLiT && fSimLiTSource) {
//     //====================================================================//
//     // SimLiT mode: Generate neutron from 7Li(p,n)7Be reaction
//     //====================================================================//
    
//     double energy_keV;  // neutron energy in keV
//     double theta_rad;   // neutron angle in radians (from beam axis)
    
//     // Generate neutron energy and angle from SimLiT
//     fSimLiTSource->GenerateNeutron(energy_keV, theta_rad);
    
//     // Store for analysis access
//     fNeutronEnergy = energy_keV;
//     fNeutronTheta = theta_rad;
    
//     //========================================================================//
//     // Convert SimLiT energy (keV) to Geant4 internal units
//     // SimLiT returns energy in keV; Geant4 expects MeV internally
//     // The 'keV' constant performs the conversion: energy_keV * keV = value_in_MeV
//     //========================================================================//
//     G4double energy = energy_keV * keV;
    
//     //====================================================================//
//     // Generate random azimuthal angle (phi)
//     // SimLiT provides polar angle (theta), phi is uniform in [0, 2*pi]
//     //====================================================================//
//     G4double phi = twopi * G4UniformRand();
    
//     //====================================================================//
//     // Calculate momentum direction from theta and phi
//     // theta is measured from the z-axis (beam direction)
//     //====================================================================//
//     G4double sinTheta = std::sin(theta_rad);
//     G4double cosTheta = std::cos(theta_rad);
    
//     G4ThreeVector direction(
//       sinTheta * std::cos(phi),
//       sinTheta * std::sin(phi),
//       cosTheta
//     );
    
//     //====================================================================//
//     // Set neutron origin position
//     // z = 0 corresponds to the Li target position
//     //====================================================================//
//     // G4ThreeVector position(0., 0., 0.);
    
//     //====================================================================//
//     // Configure particle gun and generate vertex
//     //====================================================================//
//     fParticleGun->SetParticleEnergy(energy);
//     fParticleGun->SetParticleMomentumDirection(direction);
//     // fParticleGun->SetParticlePosition(position);
//     fParticleGun->GeneratePrimaryVertex(anEvent);
//   }
//   else {
//     //====================================================================//
//     // Simple mode: Use particle gun defaults or user-configured values
//     // Generates isotropic neutrons in forward hemisphere for testing
//     //====================================================================//
    
//     // Generate isotropic direction in forward hemisphere
//     G4double cosTheta = G4UniformRand();  // cos(theta) uniform in [0, 1]
//     G4double sinTheta = std::sqrt(1. - cosTheta*cosTheta);
//     G4double phi = twopi * G4UniformRand();
    
//     // G4ThreeVector direction(
//     //   sinTheta * std::cos(phi),
//     //   sinTheta * std::sin(phi),
//     //   cosTheta
//     // );

//     G4ThreeVector direction(0,0,1); // All neutrons along z-axis for simple mode test
    
//     fParticleGun->SetParticleMomentumDirection(direction);
//     fParticleGun->GeneratePrimaryVertex(anEvent);
    
//     // Store for consistency
//     // fNeutronEnergy = fParticleGun->GetParticleEnergy();
//     fNeutronEnergy = 50.0;
//     fNeutronTheta = std::acos(cosTheta);
//   }
// }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//                         SimLiT Parameter Methods
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// void PrimaryGeneratorAction::SetUseSimLiT(G4bool use)
// {
//   //========================================================================//
//   // Enable/disable SimLiT neutron source
//   //========================================================================//
//   fUseSimLiT = use;
  
//   if (fUseSimLiT) {
//     G4cout << "SimLiT neutron source enabled." << G4endl;
//   }
//   else {
//     G4cout << "SimLiT neutron source disabled. Using linear particle gun." << G4endl;
//   }
// }

// // G4bool PrimaryGeneratorAction::GetUseSimLiT() const
// // {
// //   return fUseSimLiT;
// // }

// void PrimaryGeneratorAction::SetBeamEnergy(G4double energy)
// {
//   //========================================================================//
//   // Set proton beam energy for SimLiT source if SimLiT enabled or 
//   // particle gun energy if SimLiT disabled
//   //========================================================================//
//   if (fSimLiTSource) {
//     fSimLiTSource->SetBeamEnergy(energy);
//     G4cout << "SimLiT beam energy set to " << energy << " keV" << G4endl;
//   }
//   else {
//     fParticleGun->SetParticleEnergy(energy * keV);
//     G4cout << "Particle gun energy set to " << energy << " keV" << G4endl;
//   }
// }

// G4double PrimaryGeneratorAction::GetBeamEnergy() const
// {
//   //========================================================================//
//   // Get proton beam energy for SimLiT source if SimLiT enabled or
//   // particle gun energy if SimLiT disabled
//   //========================================================================//
//   if (fSimLiTSource) {
//     return fSimLiTSource->GetBeamEnergy();
//   }
//   else {
//     return fParticleGun->GetParticleEnergy() / keV;
//   }
// }

// //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// void PrimaryGeneratorAction::SetBeamSigma(G4double sigma)
// {
//   //========================================================================//
//   // Set proton beam energy spread for SimLiT source (in keV)
//   //========================================================================//
//   if (fSimLiTSource) {
//     fSimLiTSource->SetBeamSigma(sigma);
//     G4cout << "SimLiT beam sigma set to " << sigma << " keV" << G4endl;
//   }
//   else {
//     G4cout << "SimLiT source not initialized." << G4endl;
//   }
// }

// G4double PrimaryGeneratorAction::GetBeamSigma() const
// {
//   if (fSimLiTSource) {
//     return fSimLiTSource->GetBeamSigma();
//   }
//   return 0.;
// }

// //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// void PrimaryGeneratorAction::SetTargetMaterial(const G4String& material)
// {
//   //========================================================================//
//   // Set lithium target composition
//   // Maps material name string to SimLiT composition index
//   //========================================================================//
//   if (!fSimLiTSource) {
//     G4cout << "SimLiT source not initialized." << G4endl;
//     return;
//   }
  
//   int composition = -1;
  
//   if (material == "Li") {
//     composition = SimLiT::Li;
//     fTargetMaterialName = "Li";
//   }
//   else if (material == "LiF") {
//     composition = SimLiT::LiF;
//     fTargetMaterialName = "LiF";
//   }
//   else if (material == "Li2O") {
//     composition = SimLiT::Li2O;
//     fTargetMaterialName = "Li2O";
//   }
//   else if (material == "Li3N") {
//     composition = SimLiT::Li3N;
//     fTargetMaterialName = "Li3N";
//   }
//   else if (material == "LiOH") {
//     composition = SimLiT::LiOH;
//     fTargetMaterialName = "LiOH";
//   }
//   else if (material == "LiH") {
//     composition = SimLiT::LiH;
//     fTargetMaterialName = "LiH";
//   }
//   else {
//     G4cerr << "Unknown target material: " << material << G4endl;
//     G4cerr << "Available: Li, LiF, Li2O, Li3N, LiOH, LiH" << G4endl;
//     return;
//   }
  
//   fSimLiTSource->SetComposition(composition);
//   G4cout << "SimLiT target material set to " << fTargetMaterialName << G4endl;
// }

// G4String PrimaryGeneratorAction::GetTargetMaterial() const
// {
//   return fTargetMaterialName;
// }

// //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// void PrimaryGeneratorAction::SetTargetThickness(G4double thickness)
// {
//   //========================================================================//
//   // Set target thickness in micrometers
//   // SimLiT stores thickness in cm internally
//   //========================================================================//
//   if (fSimLiTSource) {
//     // Convert um to cm for SimLiT
//     fSimLiTSource->SetTargetThickness(thickness * SimLiT::um);
//     G4cout << "SimLiT target thickness set to " << thickness << " um" << G4endl;
//   }
//   else {
//     G4cout << "SimLiT source not initialized." << G4endl;
//   }
// }

// G4double PrimaryGeneratorAction::GetTargetThickness() const
// {
//   if (fSimLiTSource) {
//     // Convert cm back to um
//     return fSimLiTSource->GetTargetThickness() / SimLiT::um;
//   }
//   return 0.;
// }

// //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// void PrimaryGeneratorAction::PrintParameters() const
// {
//   //========================================================================//
//   // Print current source configuration
//   //========================================================================//
  
//   G4cout << G4endl;
//   G4cout << "========================================" << G4endl;
//   G4cout << "Primary Generator Configuration:" << G4endl;
//   G4cout << "========================================" << G4endl;
  
//   if (fUseSimLiT && fSimLiTSource) {
//     G4cout << "  Source mode:      SimLiT 7Li(p,n)7Be" << G4endl;
//     G4cout << "  Proton beam:" << G4endl;
//     G4cout << "    Energy:         " << fSimLiTSource->GetBeamEnergy() << " keV" << G4endl;
//     G4cout << "    Sigma:          " << fSimLiTSource->GetBeamSigma() << " keV" << G4endl;
//     G4cout << "  Target:" << G4endl;
//     G4cout << "    Material:       " << fTargetMaterialName << G4endl;
//     G4cout << "    Thickness:      " << GetTargetThickness() << " um" << G4endl;
//     G4cout << "  Reaction threshold: 1880.4 keV" << G4endl;
    
//     // // Estimate peak neutron energy at 0 degrees
//     // G4double Ep = fSimLiTSource->GetBeamEnergy();
//     // G4double Eth = 1880.36;  // keV
//     // if (Ep > Eth) {
//     //   // Rough estimate: En(0) ~ (Ep - Eth) * (Mn/(Mp+Mn))^2 * factor
//     //   // This is approximate - actual depends on kinematics
//     //   G4double En_est = 0.475 * (Ep - Eth);  // Empirical factor
//     //   G4cout << "  Est. peak neutron E (0 deg): ~" << En_est << " keV" << G4endl;
//     // }
//   }
//   else {
//     G4cout << "  Source mode:      Simple particle gun" << G4endl;
//     G4cout << "  Particle:         " 
//            << fParticleGun->GetParticleDefinition()->GetParticleName() << G4endl;
//     G4cout << "  Energy:           " 
//            << fParticleGun->GetParticleEnergy()/MeV << " MeV" << G4endl;
//     G4cout << "  Direction:        Isotropic (forward hemisphere)" << G4endl;
//   }
  
//   G4cout << "========================================" << G4endl;
//   G4cout << G4endl;
// }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
