//
// DetectorSD.cc
// Implementation of the DetectorSD class
// This is where energy depositions in the Li-6 glass are recorded
//

#include "DetectorSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4VProcess.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"
#include "G4ios.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorSD::DetectorSD(const G4String& name, const G4String& hitsCollectionName)
 : G4VSensitiveDetector(name)
{
  // Register the hits collection name
  // This name is used to retrieve the collection in EventAction
  collectionName.insert(hitsCollectionName);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorSD::Initialize(G4HCofThisEvent* hce)
{
  // Called at the beginning of each event
  // Creates a new hits collection for this event
  
  fHitsCollection = new DetectorHitsCollection(SensitiveDetectorName, collectionName[0]);

  // Add this collection to the event
  // G4SDManager automatically assigns a unique ID
  G4int hcID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
  hce->AddHitsCollection(hcID, fHitsCollection);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4bool DetectorSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
  // Called by Geant4 for EVERY step inside the sensitive volume
  // This is where we record energy depositions
  
  // Get energy deposit for this step
  // G4double edep = step->GetTotalEnergyDeposit();
  G4double edep = 0.0;
  
  // Get step length (for charged particles only)
  G4double stepLength = 0.;
  if (step->GetTrack()->GetDefinition()->GetPDGCharge() != 0.) {
    stepLength = step->GetStepLength();
  }

  //========================================================================//
  // Get the process that defined this step
  // For Li-6(n,alpha)triton reactions via NeutronHP:
  //   - Process name is typically "nCapture" for thermal captures
  //   - May also see "neutronInelastic" for higher-energy reactions
  // EventAction checks for "nCapture" specifically
  // ========================================================================//
  const G4VProcess* process = step->GetPostStepPoint()->GetProcessDefinedStep();
  G4String processName = "";
  if (process) {
    processName = process->GetProcessName();
  }

  G4bool recordHit = false;
  
  // Always record if there's energy deposition
  if (edep > 0. || stepLength > 0.) {
    recordHit = true;
  }

  // Get particle name
  G4String particleName = step->GetTrack()->GetDefinition()->GetParticleName();
  
  // Also record neutron interactions even without edep (for visualization)
  if (particleName == "neutron") {
    if (processName == "hadElastic" || 
        processName == "nCapture" || 
        processName == "neutronInelastic" ||
        processName == "nFission") {
      recordHit = true;
    }
  }
  
  if (!recordHit) return false;

  // Skip steps with no energy deposit and no track length
  // if (edep == 0. && stepLength == 0.) return false;

  // Create a new hit for this step
  DetectorHit* newHit = new DetectorHit();

  // Fill hit with step information
  newHit->SetTrackID(step->GetTrack()->GetTrackID());
  newHit->SetEdep(edep);
  newHit->SetTrackLength(stepLength);
  newHit->SetPos(step->GetPostStepPoint()->GetPosition());
  newHit->SetTime(step->GetPostStepPoint()->GetGlobalTime());
  newHit->SetParticleType(particleName);
  newHit->SetProcessName(processName);

  // Add hit to the collection
  fHitsCollection->insert(newHit);

  return true;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorSD::EndOfEvent(G4HCofThisEvent*)
{
  // Called at the end of each event
  // Print hits if verbosity is high (for debugging)
  
  if (verboseLevel > 1) {
    G4int nofHits = fHitsCollection->entries();
    G4cout 
      << G4endl
      << "-------->Hits Collection: in this event there are " << nofHits
      << " hits in the Li-6 detector:" << G4endl;
    
    for (G4int i = 0; i < nofHits; i++) {
      (*fHitsCollection)[i]->Print();
    }
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
