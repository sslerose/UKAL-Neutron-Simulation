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

G4Run* RunAction::GenerateRun()
{
  fRun = new Run(fDetector);
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Actions to perform at the beginning of each run
//------------------------------------------------------------------------//
void RunAction::BeginOfRunAction(const G4Run* run)
{
  // Show random number engine status
  if (isMaster) {
    G4Random::showEngineStatus();
    Run::InitProgressTracking(run->GetNumberOfEventToBeProcessed());
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
  analysisManager->OpenFile();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Actions to perform at the end of each run
//------------------------------------------------------------------------//
void RunAction::EndOfRunAction(const G4Run*)
{
  // Print neutron transport statistics (from Hadr04 Run class)
  if (isMaster) fRun->EndOfRun();

  // Print detector analysis statistics
  auto analysisManager = G4AnalysisManager::Instance();
  if (analysisManager->IsOpenFile()) {
    if (isMaster) {
      fDetector->PrintParameters();

      G4cout << G4endl;
      G4cout << "============================================================" << G4endl;
      G4cout << "                       End of Run Action                    " << G4endl;
      G4cout << "============================================================" << G4endl;

    //   // Source statistics
    //   G4cout << "Neutron Source:" << G4endl;
    //   G4cout << "  Mode: "
    //          << (PrimaryGeneratorConfig::Instance()->GetUseSimLiT() ? "SimLiT" : "Particle Gun")
    //          << G4endl;
    //   G4cout << "  Total neutrons generated: " << analysisManager->GetH1(0)->entries() << G4endl;
    //   if (analysisManager->GetH1(0)->entries() > 0) {
    //     G4cout << "  Energy: mean = " << analysisManager->GetH1(0)->mean()
    //           << " keV, RMS = " << analysisManager->GetH1(0)->rms() << " keV" << G4endl;
    //   }
    //   if (analysisManager->GetH1(1)->entries() > 0) {
    //     G4cout << "  Angle:  mean = " << analysisManager->GetH1(1)->mean()
    //           << " deg, RMS = " << analysisManager->GetH1(1)->rms() << " deg" << G4endl;
    //   }
    }


    // Save histograms and ntuple
    analysisManager->Write();
    analysisManager->CloseFile(false);
  }

  // Reset progress tracking and show random number engine status
  if (isMaster) {
    Run::ResetProgressTracking();
    G4Random::showEngineStatus();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
