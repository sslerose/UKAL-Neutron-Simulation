//
// EventAction.cc
// Implementation of the EventAction class
// Analyzes detector hits and fills histograms
//

#include "EventAction.hh"
#include "DetectorHit.hh"
#include "PrimaryGeneratorAction.hh"
#include "HistoManager.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4RunManager.hh"

#include <iomanip>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

EventAction::EventAction(PrimaryGeneratorAction* primaryGen)
  : fPrimaryGenerator(primaryGen)
{

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorHitsCollection* EventAction::GetHitsCollection(G4int hcID, const G4Event* event) const
{
  // Retrieve hits collection from the event
  // Called once per event to get the detector hits
  
  auto hitsCollection = static_cast<DetectorHitsCollection*>(
    event->GetHCofThisEvent()->GetHC(hcID));

  if (!hitsCollection) {
    G4ExceptionDescription msg;
    msg << "Cannot access hitsCollection ID " << hcID;
    G4Exception("EventAction::GetHitsCollection()",
                "MyCode0003", FatalException, msg);
  }

  return hitsCollection;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::BeginOfEventAction(const G4Event*)
{
  // Called at the beginning of each event
  // Currently empty - could be used for initialization if needed
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::AnalyzeHits(DetectorHitsCollection* hc)
{
  //========================================================================//
  // Initialize event variables
  //========================================================================//
  G4double totalEdep = 0.;
  G4double captureTime = -1.0;  // TOF: time of neutron capture (-1 = no capture)
  // G4double firstHitTime = 1.0e10;  // Initialize to large time
  G4bool captureOccurred = false;
  G4int nNeutronHits = 0;
  // G4int nTritonHits = 0;
  // G4int nAlphaHits = 0;
  G4int nCaptureEvents = 0;
  G4double TOF_Energy = 0.;

  //========================================================================//
  // Get primary neutron parameters from SimLiT
  //========================================================================//
  G4double neutronEnergy_keV = 0.;
  G4double neutronTheta_deg = 0.;
  
  if (fPrimaryGenerator) {
    neutronEnergy_keV = fPrimaryGenerator->GetNeutronEnergy();
    // Convert theta from radians to degrees for ntuple
    neutronTheta_deg = fPrimaryGenerator->GetNeutronTheta() * 180.0 / CLHEP::pi;
  }
  
  //========================================================================//
  // Analyze hits in the detector
  //========================================================================//
  G4int nHits = hc->entries();
  // if (nHits == 0) return;  // No hits in this event

  // Loop through all hits in this event
  if (nHits > 0) {
    for (G4int i = 0; i < nHits; i++) {
      DetectorHit* hit = (*hc)[i];
      
      // Accumulate total energy
      totalEdep += hit->GetEdep();
      
      // Count particle types
      G4String particleType = hit->GetParticleType();
      if (particleType == "neutron") nNeutronHits++;
      // else if (particleType == "triton") nTritonHits++;
      // else if (particleType == "alpha") nAlphaHits++;
      
      //====================================================================//
      // Check for neutron capture event
      // The nCapture process marks where n + Li-6 → alpha + triton occurs
      // This is the detection event that triggers scintillation
      // Global time = time since event start, includes neutron flight time
      //
      // Note: We take the FIRST capture in case of multiple
      // (rare, but possible with multiple neutrons)
      //====================================================================//
      if (hit->GetProcessName() == "nCapture" && !captureOccurred) {
        nCaptureEvents++;
        captureTime = hit->GetTime();
        captureOccurred = true;
      }
    }
  }

  //========================================================================//
  // Fill histograms
  // 
  // G4AnalysisManager histogram IDs are assigned sequentially:
  //   ID 0: NeutronEnergy - Primary neutron energy distribution
  //   ID 1: NeutronTheta  - Primary neutron angle distribution  
  //   ID 2: TotalEdep     - Energy deposited in detector
  //   ID 3: TOF           - Time of flight (capture events only)
  //   ID 4: EdepCapture   - Energy deposited when capture occurs
  //========================================================================//

  G4double n_velocity = 0.51 / (captureTime / ns) * 1e9; // in m/s
  TOF_Energy = 0.5 * (939.57 / (3e8 * 3e8)) * (n_velocity * n_velocity) * 1000; // in keV

  auto analysisManager = G4AnalysisManager::Instance();
  
  // Always fill primary neutron distributions (every event)
  analysisManager->FillH1(HistoManager::kH_NeutronEnergy, neutronEnergy_keV);
  analysisManager->FillH1(HistoManager::kH_NeutronTheta, neutronTheta_deg);
  
  // Fill TOF only for capture events (Option B measurement)
  if (captureOccurred) {
    analysisManager->FillH1(HistoManager::kH_nCapture, nCaptureEvents);
    analysisManager->FillH1(HistoManager::kH_TOF, captureTime / ns);
    analysisManager->FillH1(HistoManager::kH_TOFEnergy, TOF_Energy);
  }

  // Fill number of neutron hits histogram
  analysisManager->FillH1(HistoManager::kH_nHits, nNeutronHits);

  //========================================================================//
  // Fill ntuple with event-by-event data
  //
  // Ntuple structure (created in RunAction):
  //   Column 0: NeutronEnergy  (Double, keV)   - SimLiT neutron energy
  //   Column 1: NeutronTheta   (Double, deg)   - SimLiT neutron angle
  //   Column 2: TotalEdep      (Double, MeV)   - Energy deposited in detector
  //   Column 3: TOF            (Double, ns)    - Time of capture (-1 if none)
  //   Column 4: CaptureFlag    (Integer)       - 1 if capture occurred, 0 otherwise
  //
  // This structure enables correlation analysis:
  //   - NeutronEnergy vs TOF (validates E = m*d²/2t² relationship)
  //   - NeutronEnergy vs TotalEdep (detection efficiency vs energy)
  //   - NeutronTheta vs CaptureFlag (angular acceptance)
  //========================================================================//
  analysisManager->FillNtupleDColumn(HistoManager::kNT_NeutronEnergy, neutronEnergy_keV);
  analysisManager->FillNtupleDColumn(HistoManager::kNT_NeutronTheta, neutronTheta_deg);
  // analysisManager->FillNtupleDColumn(HistoManager::kNT_TotalEdep, totalEdep / MeV);
  analysisManager->FillNtupleDColumn(HistoManager::kNT_TOF, captureTime / ns);  // -1 if no capture
  analysisManager->FillNtupleIColumn(HistoManager::kNT_CaptureFlag, captureOccurred ? 1 : 0);
  analysisManager->AddNtupleRow();

  // Print statistics for this event (if requested)
  auto eventID = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
  auto printModulo = G4RunManager::GetRunManager()->GetPrintProgress();
  // if ((printModulo > 0) && (eventID % printModulo == 0)) {
  //   PrintEventStatistics(totalEdep, captureTime, nHits);
  // }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::EndOfEventAction(const G4Event* event)
{
  // Called at the end of each event
  // Get the hits collection and analyze it
  
  // Get hits collection ID (only need to do this once)
  // if (fDetectorHCID == -1) {
  //   fDetectorHCID = G4SDManager::GetSDMpointer()->GetCollectionID("DetectorHitsCollection");
  // }

  // // Get hits collection for this event
  // auto hc = GetHitsCollection(fDetectorHCID, event);

  // // Analyze the hits
  // AnalyzeHits(hc);

  //========================================================================//
  // Retrieve and analyze hits from the sensitive detector
  //========================================================================//
  
  // Get hits collection ID (cached after first event)
  if (fDetectorHCID == -1) {
    fDetectorHCID = G4SDManager::GetSDMpointer()->GetCollectionID("DetectorHitsCollection");
    
    // Validate that the collection was found
    if (fDetectorHCID == -1) {
      G4ExceptionDescription msg;
      msg << "DetectorHitsCollection not found. "
          << "Check that DetectorSD is properly attached in ConstructSDandField().";
      G4Exception("EventAction::EndOfEventAction()",
                  "NeutronDet001", JustWarning, msg);
      return;
    }
  }

  // Get hits collection - GetHitsCollection handles null case with FatalException
  auto hitsCollection = event->GetHCofThisEvent();
  if (!hitsCollection) {
    return;  // No hits collections in this event (geometry issue)
  }
  
  auto hc = GetHitsCollection(fDetectorHCID, event);
  if (hc) {
    AnalyzeHits(hc);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void EventAction::PrintEventStatistics(G4double totalEdep, G4double tof, G4int nHits) const
{
  // Print event summary for periodic monitoring
  
  G4cout 
    << "Event Statistics:"
    // << "\n  Total Energy Deposited: " << std::setw(7) << G4BestUnit(totalEdep, "Energy")
    << "\n  Time of Flight: " << std::setw(7) << G4BestUnit(tof, "Time")
    << "\n  Number of Hits: " << nHits
    << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
