//
// DetectorHit.cc
// Implementation of the DetectorHit class
//

#include "DetectorHit.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"

#include <iomanip>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

// Thread-local allocator initialization
G4ThreadLocal G4Allocator<DetectorHit>* DetectorHitAllocator;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorHit::Draw()
{
  //========================================================================//
  // Visualization of hits as colored markers
  //
  // Color scheme:
  //   - Blue:  General hits (neutron scattering, energy deposition)
  //   - Red:   Neutron capture events (Li-6(n,alpha)triton)
  //
  // This allows visual identification of where captures occur
  // versus where neutrons simply scatter in the detector
  //========================================================================//
  
  G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
  if (!pVVisManager) return;

  // Create a circle marker at the hit position
  G4Circle circle(fPos);
  circle.SetScreenSize(8.);        // Size in pixels
  circle.SetFillStyle(G4Circle::filled);
  
  // Set color based on process type
  G4Colour colour;
  if (fProcessName == "nCapture") {
    // Red for neutron capture events
    colour = G4Colour::Red();  // Red
  }
  else {
    // Blue for all other interactions (scattering, etc.)
    colour = G4Colour::Blue();  // Blue
  }
  
  G4VisAttributes attribs(colour);
  circle.SetVisAttributes(attribs);
  
  pVVisManager->Draw(circle);
}

void DetectorHit::Print()
{
  // Print hit information in human-readable format
  // Useful for debugging and verification
  
  G4cout 
    << "  TrackID: " << std::setw(5) << fTrackID
    << "  Particle: " << std::setw(10) << fParticleType
    << "  Edep: " << std::setw(7) << G4BestUnit(fEdep, "Energy")
    << "  Time: " << std::setw(7) << G4BestUnit(fTime, "Time")
    << "  Process: " << fProcessName
    << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
