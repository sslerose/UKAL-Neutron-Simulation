//
// EventAction.hh
// Event action class for neutron detection analysis
// Processes hits from sensitive detector and fills histograms
//

#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "DetectorHit.hh"
#include "globals.hh"

class G4Event;
class PrimaryGeneratorAction;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

/// Event action class
///
/// Processes hits at the end of each event to extract physics information:
/// - Total energy deposited (should be ~4.78 MeV for neutron captures)
/// - Time-of-flight from neutron generation to first detector hit
/// - Particle contributions to energy deposition
/// - Process identification (to count neutron captures)
///
/// This class bridges between the sensitive detector (which records hits)
/// and the analysis manager (which creates histograms)

class EventAction : public G4UserEventAction
{
  public:
    // Constructor requires PrimaryGeneratorAction pointer to access
    // SimLiT neutron energy and angle for each event
    EventAction(PrimaryGeneratorAction*);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

  private:
    // Helper methods
    DetectorHitsCollection* GetHitsCollection(G4int, const G4Event*) const;
    void AnalyzeHits(DetectorHitsCollection*);
    void PrintEventStatistics(G4double, G4double, G4int) const;

    // Data members
    G4int fDetectorHCID = -1;  // Hits collection ID (initialized once)
    PrimaryGeneratorAction* fPrimaryGenerator = nullptr; // Pointer to primary generator
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
