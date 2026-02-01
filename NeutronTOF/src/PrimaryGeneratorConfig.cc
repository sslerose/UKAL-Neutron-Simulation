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
/// \file PrimaryGeneratorConfig.cc
/// \brief Implementation of the PrimaryGeneratorConfig class
//

#include "PrimaryGeneratorConfig.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Initialize static members
PrimaryGeneratorConfig* PrimaryGeneratorConfig::fInstance = nullptr;
G4Mutex PrimaryGeneratorConfig::fInstanceMutex = G4MUTEX_INITIALIZER;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorConfig* PrimaryGeneratorConfig::Instance()
{
  G4AutoLock lock(&fInstanceMutex);
  if (fInstance == nullptr) {
    fInstance = new PrimaryGeneratorConfig();
  }
  return fInstance;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorConfig::PrimaryGeneratorConfig()
  : fUseSimLiT(true),           // Default: SimLiT disabled (matches your current code)
    fBeamEnergy(1912.0),         // keV
    fBeamSigma(10.0),            // keV
    fTargetMaterial("LiF"),
    fTargetThickness(10.0),      // micrometers
    fGunEnergy(50.0*keV),        // Default gun energy
    fLimitToDetector(false),     // Default: generate neutrons in all directions
    fOutputFileName(""),
    fHasOutputFileName(false),
    fMutex(G4MUTEX_INITIALIZER)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get use of SimLiT neutron source
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetUseSimLiT(G4bool use)
{
  G4AutoLock lock(&fMutex);
  fUseSimLiT = use;
  
  // Output from master thread (command execution)
  if (use) {
    G4cout << "SimLiT neutron source ENABLED" << G4endl;
  } else {
    G4cout << "SimLiT neutron source DISABLED - using simple particle gun" << G4endl;
  }
}

G4bool PrimaryGeneratorConfig::GetUseSimLiT() const
{
  G4AutoLock lock(&fMutex);
  return fUseSimLiT;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get SimLiT proton beam energy (in keV)
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetBeamEnergy(G4double energy)
{
  G4AutoLock lock(&fMutex);
  fBeamEnergy = energy;
  G4cout << "SimLiT beam energy set to " << energy << " keV" << G4endl;
}

G4double PrimaryGeneratorConfig::GetBeamEnergy() const
{
  G4AutoLock lock(&fMutex);
  return fBeamEnergy;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get SimLiT proton beam sigma (in keV)
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetBeamSigma(G4double sigma)
{
  G4AutoLock lock(&fMutex);
  fBeamSigma = sigma;
  G4cout << "SimLiT beam sigma set to " << sigma << " keV" << G4endl;
}

G4double PrimaryGeneratorConfig::GetBeamSigma() const
{
  G4AutoLock lock(&fMutex);
  return fBeamSigma;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get SimLiT Lithium target material
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetTargetMaterial(const G4String& material)
{
  G4AutoLock lock(&fMutex);
  fTargetMaterial = material;
  G4cout << "SimLiT target material set to " << material << G4endl;
}

G4String PrimaryGeneratorConfig::GetTargetMaterial() const
{
  G4AutoLock lock(&fMutex);
  return fTargetMaterial;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get SimLiT Lithium target thickness (in micrometers)
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetTargetThickness(G4double thickness)
{
  G4AutoLock lock(&fMutex);
  fTargetThickness = thickness;
  G4cout << "SimLiT target thickness set to " << thickness << " um" << G4endl;
}

G4double PrimaryGeneratorConfig::GetTargetThickness() const
{
  G4AutoLock lock(&fMutex);
  return fTargetThickness;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get particle gun energy (when SimLiT disabled)
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetGunEnergy(G4double energy)
{
  G4AutoLock lock(&fMutex);
  fGunEnergy = energy;
  G4cout << "Particle gun energy set to " << G4BestUnit(energy, "Energy") << G4endl;
}

G4double PrimaryGeneratorConfig::GetGunEnergy() const
{
  G4AutoLock lock(&fMutex);
  return fGunEnergy;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set/Get detector-focused generation mode
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetLimitToDetector(G4bool limit)
{
  G4AutoLock lock(&fMutex);
  fLimitToDetector = limit;

  if (limit) {
    G4cout << "Detector-focused neutron generation ENABLED" << G4endl;
    G4cout << "  Neutrons will only be generated toward the detector" << G4endl;
  } else {
    G4cout << "Detector-focused neutron generation DISABLED" << G4endl;
    G4cout << "  Neutrons will be generated in all directions" << G4endl;
  }
}

G4bool PrimaryGeneratorConfig::GetLimitToDetector() const
{
  G4AutoLock lock(&fMutex);
  return fLimitToDetector;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Output filename management
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::SetOutputFileName(const G4String& name)
{
  G4AutoLock lock(&fMutex);
  fOutputFileName = name;
  fHasOutputFileName = true;
}

G4String PrimaryGeneratorConfig::GetOutputFileName() const
{
  G4AutoLock lock(&fMutex);
  return fOutputFileName;
}

G4bool PrimaryGeneratorConfig::HasOutputFileName() const
{
  G4AutoLock lock(&fMutex);
  return fHasOutputFileName;
}

void PrimaryGeneratorConfig::ClearOutputFileName()
{
  G4AutoLock lock(&fMutex);
  fOutputFileName = "";
  fHasOutputFileName = false;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Print current primary generator configuration
//------------------------------------------------------------------------//
void PrimaryGeneratorConfig::PrintParameters() const
{
  G4AutoLock lock(&fMutex);
  
  G4cout << G4endl;
  G4cout << "========================================" << G4endl;
  G4cout << "Primary Generator Configuration:" << G4endl;
  G4cout << "========================================" << G4endl;
  
  if (fUseSimLiT) {
    G4cout << "  Source mode:      SimLiT 7Li(p,n)7Be" << G4endl;
    G4cout << "  Proton beam:" << G4endl;
    G4cout << "    Energy:         " << fBeamEnergy << " keV" << G4endl;
    G4cout << "    Sigma:          " << fBeamSigma << " keV" << G4endl;
    G4cout << "  Target:" << G4endl;
    G4cout << "    Material:       " << fTargetMaterial << G4endl;
    G4cout << "    Thickness:      " << fTargetThickness << " um" << G4endl;
    G4cout << "  Reaction threshold: 1880.4 keV" << G4endl;
    if (fLimitToDetector) {
      G4cout << "  Angular limit:    Detector-focused (limited theta/phi)" << G4endl;
    } else {
      G4cout << "  Angular limit:    Full hemisphere (unrestricted)" << G4endl;
    }
  }
  else {
    G4cout << "  Source mode:      Simple particle gun" << G4endl;
    G4cout << "  Particle:         neutron" << G4endl;
    G4cout << "  Energy:           " << G4BestUnit(fGunEnergy, "Energy") << G4endl;
    G4cout << "  Direction:        Along z-axis (simple mode)" << G4endl;
  }
  
  G4cout << "========================================" << G4endl;
  G4cout << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
