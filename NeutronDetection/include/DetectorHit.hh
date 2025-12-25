//
// DetectorHit.hh
// Hit class for Li-6 glass neutron detector
// Records energy deposition, time (for TOF), position, and particle information
//

#ifndef DetectorHit_h
#define DetectorHit_h 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4Threading.hh"
#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

/// Neutron detector hit class
///
/// Stores information about energy depositions in the Li-6 glass detector.
/// Each step that deposits energy creates or updates a hit.
/// Key data for neutron detection analysis:
/// - Energy deposit (for identifying 4.78 MeV neutron captures)
/// - Time (for time-of-flight measurements)
/// - Position (for spatial distribution)
/// - Particle type (neutron, triton, alpha, etc.)

class DetectorHit : public G4VHit
{
  public:
    DetectorHit() = default;
    DetectorHit(const DetectorHit&) = default;
    ~DetectorHit() override = default;

    // Operators
    DetectorHit& operator=(const DetectorHit&) = default;
    G4bool operator==(const DetectorHit&) const;

    inline void* operator new(size_t);
    inline void  operator delete(void*);

    // Methods from base class
    void Draw() override;
    void Print() override;

    // Add energy and track length to this hit
    void Add(G4double de, G4double dl);

    // Setters
    void SetTrackID(G4int track)              { fTrackID = track; }
    void SetEdep(G4double de)                 { fEdep = de; }
    void SetTrackLength(G4double dl)          { fTrackLength = dl; }
    void SetPos(G4ThreeVector xyz)            { fPos = xyz; }
    void SetTime(G4double t)                  { fTime = t; }
    void SetParticleType(G4String type)       { fParticleType = type; }
    void SetProcessName(G4String process)     { fProcessName = process; }

    // Getters
    G4int GetTrackID() const                  { return fTrackID; }
    G4double GetEdep() const                  { return fEdep; }
    G4double GetTrackLength() const           { return fTrackLength; }
    G4ThreeVector GetPos() const              { return fPos; }
    G4double GetTime() const                  { return fTime; }
    G4String GetParticleType() const          { return fParticleType; }
    G4String GetProcessName() const           { return fProcessName; }

  private:
    G4int         fTrackID = -1;        ///< Track ID number
    G4double      fEdep = 0.;           ///< Energy deposit in the detector
    G4double      fTrackLength = 0.;    ///< Track length in the detector
    G4ThreeVector fPos;                 ///< Position of the hit
    G4double      fTime = 0.;           ///< Global time of the hit (for TOF)
    G4String      fParticleType = "";   ///< Particle type causing the hit
    G4String      fProcessName = "";    ///< Process that created this step
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Define hits collection type
// using DetectorHitsCollection = G4THitsCollection<DetectorHit>;
using DetectorHitsCollection = G4THitsCollection<DetectorHit>;

// Thread-local allocator for efficient memory management
extern G4ThreadLocal G4Allocator<DetectorHit>* DetectorHitAllocator;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Inline implementations for efficiency

inline void* DetectorHit::operator new(size_t)
{
  if (!DetectorHitAllocator) {
    DetectorHitAllocator = new G4Allocator<DetectorHit>;
  }
  void* hit = (void*) DetectorHitAllocator->MallocSingle();
  return hit;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

inline void DetectorHit::operator delete(void* hit)
{
  DetectorHitAllocator->FreeSingle((DetectorHit*) hit);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

inline void DetectorHit::Add(G4double de, G4double dl)
{
  // Accumulate energy and track length
  // This is called when multiple steps occur in the same hit
  fEdep += de;
  fTrackLength += dl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

inline G4bool DetectorHit::operator==(const DetectorHit& right) const
{
  // Compare by identity (pointer equality)
  // Two hits are equal only if they are the same object
  return (this == &right) ? true : false;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
