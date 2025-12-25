//
// RunAction.cc (UPDATED for neutron detection)
// Combines Hadr04's neutron transport tracking with B4c's detector analysis
// Creates histograms and ntuples for neutron detection analysis
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
  // Create custom Run object (from Hadr04)
  // This accumulates neutron transport statistics
  fRun = new Run(fDetector);
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run*)
{
  // Show random number engine status
  if (isMaster) G4Random::showEngineStatus();

  // Store primary particle info in Run object
  if (fPrimary) {
    G4ParticleDefinition* particle = fPrimary->GetParticleGun()->GetParticleDefinition();
    G4double energy = fPrimary->GetParticleGun()->GetParticleEnergy();
    fRun->SetPrimary(particle, energy);
  }

  // Open analysis manager output file
  auto analysisManager = G4AnalysisManager::Instance();
  if (analysisManager->IsActive()) {
    analysisManager->Reset();
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
    
    // // Print histogram statistics
    // G4cout << G4endl << " ----> Detector Analysis Histograms:" << G4endl;
    
    // if (analysisManager->GetH1(8)) {
    //   G4cout << " Total Edep: mean = " 
    //          << G4BestUnit(analysisManager->GetH1(8)->mean(), "Energy")
    //          << " rms = " 
    //          << G4BestUnit(analysisManager->GetH1(8)->rms(), "Energy") 
    //          << G4endl;
    // }
    
    // if (analysisManager->GetH1(9)) {
    //   G4cout << " TOF: mean = " 
    //          << G4BestUnit(analysisManager->GetH1(9)->mean(), "Time")
    //          << " rms = " 
    //          << G4BestUnit(analysisManager->GetH1(9)->rms(), "Time") 
    //          << G4endl;
    // }
    
    // if (analysisManager->GetH1(10)) {
    //   G4cout << " NHits: mean = " 
    //          << analysisManager->GetH1(10)->mean()
    //          << " rms = " 
    //          << analysisManager->GetH1(10)->rms() 
    //          << G4endl;
    // }

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
  }

  // Show random number engine status
  if (isMaster) G4Random::showEngineStatus();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
