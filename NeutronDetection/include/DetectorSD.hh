//
// DetectorSD.hh
// Sensitive detector class for Li-6 glass neutron detector
// Records all energy depositions in the detector volume
//

#ifndef DetectorSD_h
#define DetectorSD_h 1

#include "G4VSensitiveDetector.hh"
#include "DetectorHit.hh"
#include "globals.hh"

class G4Step;
class G4HCofThisEvent;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

/// Sensitive detector class for neutron detector
///
/// This class is attached to the Li-6 glass logical volume in DetectorConstruction.
/// It is called automatically by Geant4 for every step inside the sensitive volume.
///
/// Key responsibilities:
/// - Create hits collection at start of event
/// - Record energy depositions from all particles (neutron, triton, alpha, etc.)
/// - Store time information for time-of-flight analysis
/// - Identify processes causing energy deposition (for neutron capture identification)
///
/// The hits collection is accessed later by EventAction to fill histograms

class DetectorSD : public G4VSensitiveDetector
{
  public:
    DetectorSD(const G4String& name, const G4String& hitsCollectionName);
    ~DetectorSD() override = default;

    // Methods from base class
    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

  private:
    DetectorHitsCollection* fHitsCollection = nullptr;  ///< Collection of hits in this event
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
