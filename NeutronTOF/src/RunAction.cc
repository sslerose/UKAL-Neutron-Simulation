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
/// \file RunAction.cc
/// \brief Implementation of the RunAction class
//

#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "HistoManager.hh"
#include "PrimaryGeneratorAction.hh"
#include "PrimaryGeneratorConfig.hh"
#include "Run.hh"

#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4UIcommand.hh"
#include "Randomize.hh"

#include <iomanip>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::RunAction(DetectorConstruction* det, PrimaryGeneratorAction* prim)
  : fDetector(det), fPrimary(prim)
{
  // Book predefined histograms from Hadr04 (neutron transport)
  fHistoManager = new HistoManager();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::~RunAction()
{
  delete fHistoManager;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4String RunAction::GenerateFileName() const
{
  //========================================================================//
  // Generate output filename based on current configuration
  // Format: nTOF<source>_<energy>keV_<angle>deg
  //========================================================================//
  
  PrimaryGeneratorConfig* config = PrimaryGeneratorConfig::Instance();
  
  // Get source type
  G4String sourceType = config->GetUseSimLiT() ? "SimLiT" : "Gun";
  
  // Get beam/gun energy
  G4double energy;
  if (config->GetUseSimLiT()) {
    energy = config->GetBeamEnergy();  // Already in keV
  } else {
    energy = config->GetGunEnergy() / keV;  // Convert from internal units to keV
  }

  G4int energyInt = static_cast<G4int>(std::round(energy));
  G4int energyDecimal = static_cast<G4int>(std::round((energy - energyInt) * 100));
  
  // Get detector angle and distance from DetectorConstruction
  G4double detectorAngle = 0.0;
  G4double detectorDistance = 0.0;
  if (fDetector) {
    detectorAngle = -(fDetector->GetDetectorAngle() / deg);
    detectorDistance = fDetector->GetDetectorDistance() / cm;
  }

  G4int angleInt = static_cast<G4int>(std::round(detectorAngle));
  G4int angleDecimal = static_cast<G4int>(std::round((detectorAngle - angleInt) * 100));

  G4int distanceInt = static_cast<G4int>(std::round(detectorDistance));
  G4int distanceDecimal = static_cast<G4int>(std::round((detectorDistance - distanceInt) * 100));
  
  // Build filename string
  std::ostringstream oss;
  oss << "nTOF_" << sourceType 
      << "_" << std::fixed << energyInt << "_" << energyDecimal << "keV"
      << "_" << std::fixed << angleInt << "_" << angleDecimal << "deg"
      << "_" << std::fixed << distanceInt << "_" << distanceDecimal << "cm";

  return oss.str();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Run* RunAction::GenerateRun()
{
  // Create custom Run object (from Hadr04)
  // This accumulates neutron transport statistics
  fRun = new Run(fDetector);
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run*)
{
  // Show random number engine status
  if (isMaster) {
    G4Random::showEngineStatus();
    G4cout << "\n===== Hello" << G4endl;
  }

  // Store primary particle info in Run object
  if (fPrimary) {
    G4ParticleDefinition* particle = fPrimary->GetParticleGun()->GetParticleDefinition();
    G4double energy = fPrimary->GetParticleGun()->GetParticleEnergy();
    fRun->SetPrimary(particle, energy);
  }

  
  //========================================================================//
  // Set up analysis manager and open output file
  //========================================================================//

  auto analysisManager = G4AnalysisManager::Instance();
  
  if (analysisManager->IsActive()) {
    PrimaryGeneratorConfig* config = PrimaryGeneratorConfig::Instance();
    G4String fileName;
    
    if (isMaster) {
      // Master generates and stores the filename
      fileName = GenerateFileName();
      config->SetOutputFileName(fileName);
      
      G4cout << "\n=== Output file: " << fileName << ".root ===" << G4endl;
      config->PrintParameters();
    }
    else {
      // Workers wait for master to set the filename
      // Spin-wait with small delay to avoid busy-waiting
      G4int waitCount = 0;
      const G4int maxWait = 1000;  // Maximum wait iterations (1000 * 1ms = 1 second max)
      
      while (!config->HasOutputFileName() && waitCount < maxWait) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        waitCount++;
      }
      
      if (config->HasOutputFileName()) {
        fileName = config->GetOutputFileName();
      } else {
        // Fallback: generate our own filename if master failed
        G4cerr << "Warning: Worker thread timed out waiting for filename from master" << G4endl;
        fileName = GenerateFileName();
      }
    }
    
    // All threads set the same filename
    analysisManager->SetFileName(fileName);
    analysisManager->OpenFile();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::EndOfRunAction(const G4Run*)
{
  // Print neutron transport statistics (from Hadr04 Run class)
  if (isMaster) fRun->EndOfRun();

  // Print detector analysis statistics
  auto analysisManager = G4AnalysisManager::Instance();
  if (analysisManager->IsActive()) {

    G4cout << G4endl;
    G4cout << "============================================================" << G4endl;
    G4cout << "                    Analysis Summary                        " << G4endl;
    G4cout << "============================================================" << G4endl;
    
    // SimLiT source statistics
    G4cout << G4endl << "SimLiT Neutron Source:" << G4endl;
    if (analysisManager->GetH1(0)->entries() > 0) {
      G4cout << "  Energy: mean = " << analysisManager->GetH1(0)->mean() 
             << " keV, RMS = " << analysisManager->GetH1(0)->rms() << " keV" << G4endl;
    }
    if (analysisManager->GetH1(1)->entries() > 0) {
      G4cout << "  Angle:  mean = " << analysisManager->GetH1(1)->mean()
             << " deg, RMS = " << analysisManager->GetH1(1)->rms() << " deg" << G4endl;
    }
    
    // Detector response statistics
    G4cout << G4endl << "Detector Response:" << G4endl;
    if (analysisManager->GetH1(2)->entries() > 0) {
      G4cout << "  Events with Edep > 0: " << analysisManager->GetH1(2)->entries() << G4endl;
      G4cout << "  Mean Edep: " << analysisManager->GetH1(2)->mean() << " MeV" << G4endl;
    }
    
    // TOF statistics (capture events)
    G4cout << G4endl << "TOF Measurement:" << G4endl;
    if (analysisManager->GetH1(3)->entries() > 0) {
      G4cout << "  Capture events: " << analysisManager->GetH1(3)->entries() << G4endl;
      G4cout << "  Mean TOF: " << analysisManager->GetH1(3)->mean() << " ns" << G4endl;
      G4cout << "  TOF RMS:  " << analysisManager->GetH1(3)->rms() << " ns" << G4endl;
    }
    
    G4cout << "============================================================" << G4endl;

    // Save histograms and ntuple
    analysisManager->Write();
    analysisManager->CloseFile(false);

    // Clear the stored filename so next run can generate a new one
    if (isMaster) {
      PrimaryGeneratorConfig::Instance()->ClearOutputFileName();
    }
  }

  // Show random number engine status
  if (isMaster) G4Random::showEngineStatus();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
