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
/// \file DetectorHit.cc
/// \brief Implementation of the DetectorHit class
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

//------------------------------------------------------------------------//
// Visualization of hits during event display
//------------------------------------------------------------------------//
void DetectorHit::Draw()
{
  //========================================================================//
  // Color scheme:
  //   - Blue:  General hits (neutron scattering, energy deposition, etc.)
  //   - Red:   6Li(n,t)4He reaction events
  //
  // Allows visual identification of captures vs scattering events
  // Also provides visual feedback on detector response during simulation
  //========================================================================//
  
  // Get pointer to visualization manager
  G4VVisManager* pVVisManager = G4VVisManager::GetConcreteInstance();
  if (!pVVisManager) return;

  // Create a circle marker at the hit position
  G4Circle circle(fPos);
  circle.SetScreenSize(8.);        // Size in pixels
  circle.SetFillStyle(G4Circle::filled);
  
  // Set color based on process type
  G4Colour colour;
  if (fProcessName == "Li6ntalpha") {
    // Red for neutron capture events
    colour = G4Colour::Red();
  }
  else {
    // Blue for all other interactions (scattering, etc.)
    colour = G4Colour::Blue();
  }
  
  // Set visualization attributes and draw
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
    << "  Time: " << std::setw(7) << G4BestUnit(fTime, "Time")
    << "  Process: " << fProcessName
    << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
