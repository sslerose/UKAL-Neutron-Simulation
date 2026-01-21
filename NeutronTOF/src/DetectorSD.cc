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
/// \file DetectorSD.cc
/// \brief Implementation of the DetectorSD class
/// This is where energy depositions in the Li-6 glass are recorded
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
  // Register the hits collection name used to retrieve the collection in EventAction
  collectionName.insert(hitsCollectionName);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Initialize hit collection at the start of each event
//------------------------------------------------------------------------//
void DetectorSD::Initialize(G4HCofThisEvent* hce)
{
  // Create hits collection for sensitive detector
  // SD name assigned by G4VSensitiveDetector(name) constructor
  // HC name assigned by collectionName.insert in constructor
  fHitsCollection = new DetectorHitsCollection(SensitiveDetectorName, collectionName[0]);

  // Add this collection to the event
  // G4SDManager automatically assigns a unique ID
  if (fHCID < 0) {
    fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
  }
  hce->AddHitsCollection(fHCID, fHitsCollection);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Process hits for each step in the sensitive volume
//------------------------------------------------------------------------//
G4bool DetectorSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
  //=======================================================================//
  // Get particle information
  //=======================================================================//

  G4Track* track = step->GetTrack();
  G4String particleName = track->GetDefinition()->GetParticleName();

  if (particleName != "neutron") {
    return false;
  }


  //=======================================================================//
  // Check if neutron is entering the detector volume
  //=======================================================================//
  
  G4StepPoint* preStepPoint = step->GetPreStepPoint();
  G4StepPoint* postStepPoint = step->GetPostStepPoint();
  
  // Check if this step crosses a geometry boundary
  G4bool isEntering = false;
  if (preStepPoint->GetStepStatus() == fGeomBoundary) {
    // Neutron crossed a boundary to enter this volume
    isEntering = true;
    
    // Create a hit to mark neutron entry
    DetectorHit* entryHit = new DetectorHit();
    entryHit->SetTrackID(track->GetTrackID());
    entryHit->SetPos(preStepPoint->GetPosition());
    entryHit->SetTime(preStepPoint->GetGlobalTime());
    entryHit->SetParticleType(particleName);
    entryHit->SetProcessName("neutronEntry");  // Special marker for entry
    entryHit->SetNeutronEntered(true);
    
    fHitsCollection->insert(entryHit);
  }


  //========================================================================//
  // Record hits for neutron interaction processes
  //========================================================================//

  // Get process that defined this step
  const G4VProcess* process = postStepPoint->GetProcessDefinedStep();
  G4String processName = "";

  if (!process) {
    return isEntering;  // Return true if entry but no process defined
  }
  
  processName = process->GetProcessName();
  
  G4bool recordInteraction = false;

  // Check for neutron interaction processes of interest
  if (processName == "hadElastic" || 
      processName == "nCapture" || 
      processName == "neutronInelastic" ||
      processName == "nFission") {
    recordInteraction = true;
  }
  
  if (!recordInteraction) {
    return isEntering;  // Return true if entry but no hit to record
  }

  // Create a new hit for this step
  DetectorHit* interactionHit = new DetectorHit();

  // Fill hit with step information
  interactionHit->SetTrackID(track->GetTrackID());
  interactionHit->SetPos(postStepPoint->GetPosition());
  interactionHit->SetTime(postStepPoint->GetGlobalTime());
  interactionHit->SetParticleType(particleName);
  interactionHit->SetProcessName(processName);
  interactionHit->SetNeutronEntered(false);  // This is an interaction, not entry

  // Add hit to the collection
  fHitsCollection->insert(interactionHit);

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
