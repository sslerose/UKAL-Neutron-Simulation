//
// StackingAction.cc
//

#include "StackingAction.hh"
#include "Run.hh"

#include "G4RunManager.hh"
#include "G4Track.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4ClassificationOfNewTrack StackingAction::ClassifyNewTrack(const G4Track* track)
{
  // Adapted from Hadr04
  // Original Hadr04 kills all secondaries to only track primary neutron
  // For neutron detection, we must track secondaries (tritons and alphas)
  // because they deposit the 4.78 MeV energy in the Li-6 glass
  
  // // Keep primary particle
  // if (aTrack->GetParentID() == 0) return fUrgent;

  // // Count secondary particles for statistics
  // G4String name = aTrack->GetDefinition()->GetParticleName();
  // G4double energy = aTrack->GetKineticEnergy();
  // Run* run = static_cast<Run*>(G4RunManager::GetRunManager()->GetNonConstCurrentRun());
  // run->ParticleCount(name, energy);

  // // Track secondaries instead of killing them
  // // Allows tritons and alphas from Li-6(n,alpha)triton to deposit energy
  // return fUrgent;  // Changed from fKill to fUrgent
  
  // Note: If you want to track ONLY particles in the detector region,
  // you could add logic here to kill particles outside the detector.
  // For example:
  //
  // G4ThreeVector pos = aTrack->GetPosition();
  // if (pos.z() < 45*cm || pos.z() > 60*cm) {
  //   return fKill;  // Kill particles far from detector
  // }
  // return fUrgent;  // Track particles near detector


  //========================================================================//
  // Keep the primary particle (TrackID == 1)
  //========================================================================//
  if (track->GetTrackID() == 1) {
    return fUrgent;
  }

  //========================================================================//
  // Kill all secondary particles
  //
  // This includes:
  //   - triton and alpha from Li-6(n,α)t capture
  //   - gammas from any reactions
  //   - electrons from ionization
  //   - any other secondaries
  //
  // For TOF-only measurement, these are not needed.
  //========================================================================//
  return fKill;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
