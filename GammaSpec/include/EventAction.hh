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
/// \file EventAction.hh
/// \brief Definition of the EventAction class
//

#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "DetectorHit.hh"
#include "globals.hh"

class G4Event;
class DetectorConstruction;
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
    EventAction(DetectorConstruction*, PrimaryGeneratorAction*);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;
    void AddEdep(G4int iVol, G4double Edep, G4double time, G4double weight);

  private:
    // Helper methods
    void PrintEventStatistics(G4double, G4int) const;

    // Data members
    DetectorConstruction* fDetector = nullptr;            // Pointer to detector construction
    PrimaryGeneratorAction* fPrimaryGenerator = nullptr;  // Pointer to primary generator
    G4double fDetDist = 0.;                               // Distance from target to detector
    G4double fEdep1 = 0., fEdep2 = 0.;                    // Energy deposition in front and back HPGe detectors
    G4double fWeight1 = 0., fWeight2 = 0.;                // Weighted energy deposition for front and back HPGe detectors
    G4double fTime0 = -1 * s;                             // Time of first energy deposition
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
