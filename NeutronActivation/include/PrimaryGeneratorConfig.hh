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
/// \file PrimaryGeneratorConfig.hh
/// \brief Definition of the PrimaryGeneratorConfig class
//

#ifndef PrimaryGeneratorConfig_h
#define PrimaryGeneratorConfig_h 1

#include "globals.hh"
#include "G4AutoLock.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class PrimaryGeneratorConfig
{
  public:
    //========================================================================//
    // Singleton access
    //========================================================================//
    static PrimaryGeneratorConfig* Instance();
    
    // Delete copy/move constructors for singleton
    PrimaryGeneratorConfig(const PrimaryGeneratorConfig&) = delete;
    PrimaryGeneratorConfig& operator=(const PrimaryGeneratorConfig&) = delete;

    //========================================================================//
    // SimLiT enable/disable
    //========================================================================//
    void SetUseSimLiT(G4bool use);
    G4bool GetUseSimLiT() const;

    //========================================================================//
    // Beam parameters (energy in keV)
    //========================================================================//
    void SetBeamEnergy(G4double energy);
    G4double GetBeamEnergy() const;

    void SetBeamSigma(G4double sigma);
    G4double GetBeamSigma() const;

    //========================================================================//
    // Target parameters
    //========================================================================//
    void SetTargetMaterial(const G4String& material);
    G4String GetTargetMaterial() const;

    void SetTargetThickness(G4double thickness);  // in micrometers
    G4double GetTargetThickness() const;

    //========================================================================//
    // Simple gun energy (when SimLiT disabled)
    //========================================================================//
    void SetGunEnergy(G4double energy);  // in Geant4 internal units
    G4double GetGunEnergy() const;

    //========================================================================//
    // Exposure time
    //========================================================================//
    void SetExposureTime(G4double time);
    G4double GetExposureTime() const;

    //========================================================================//
    // Information methods - safe to call from any thread
    //========================================================================//
    void PrintParameters() const;
    
    //========================================================================//
    // Output filename management
    // Filename is generated once on master thread, then read by all workers
    //========================================================================//
    void SetOutputFileName(const G4String& name);
    G4String GetOutputFileName() const;
    G4bool HasOutputFileName() const;
    void ClearOutputFileName();  // Call at end of run to allow new filename next run

  private:
    //========================================================================//
    // Private constructor for singleton
    //========================================================================//
    PrimaryGeneratorConfig();
    ~PrimaryGeneratorConfig() = default;

    //========================================================================//
    // Configuration parameters
    //========================================================================//
    G4bool fUseSimLiT;
    G4double fBeamEnergy;      // keV
    G4double fBeamSigma;       // keV
    G4String fTargetMaterial;
    G4double fTargetThickness; // micrometers
    G4double fGunEnergy;       // Geant4 internal units (MeV)
    G4double fTimeExposure;    // seconds

    //========================================================================//
    // Output filename (set by master, read by workers)
    //========================================================================//
    G4String fOutputFileName;
    G4bool fHasOutputFileName;

    //========================================================================//
    // Thread safety
    //========================================================================//
    mutable G4Mutex fMutex;
    
    //========================================================================//
    // Singleton instance
    //========================================================================//
    static PrimaryGeneratorConfig* fInstance;
    static G4Mutex fInstanceMutex;
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
