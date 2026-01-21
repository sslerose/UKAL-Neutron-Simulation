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
/// \file HistoManager.hh
/// \brief Definition of the HistoManager class
//

#ifndef HistoManager_h
#define HistoManager_h 1

#include "G4AnalysisManager.hh"
#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class HistoManager
{
  public:
    HistoManager();
    ~HistoManager() = default;

    //========================================================================//
    // Histogram ID constants for TOF analysis
    // These are public so EventAction can reference them
    //========================================================================//
    static constexpr G4int kH_NeutronEnergy = 0;  // Neutron energy from primary generator (keV)
    static constexpr G4int kH_NeutronTheta = 1;   // Neutron angle from primary generator (deg)
    static constexpr G4int kH_nHits = 2;          // Number of neutrons hits per event
    static constexpr G4int kH_TOF = 3;            // Time of flight (i.e., capture time) (ns)
    static constexpr G4int kH_TOFEnergy = 4;      // Neutron energy from TOF (keV)

    //========================================================================//
    // Ntuple column ID constants
    //========================================================================//
    static constexpr G4int kNT_NeutronEnergy = 0; // Neutron energy from primary generator (keV)
    static constexpr G4int kNT_NeutronTheta = 1;  // Neutron angle from primary generator (deg)
    static constexpr G4int kNT_EntryFlag = 2;   // Entry occurred flag (1=entered, 0=not entered)
    static constexpr G4int kNT_CaptureFlag = 3;   // Capture occurred flag (1=capture, 0=no capture)
    static constexpr G4int kNT_TOF = 4;           // Time of flight (i.e., capture time) (ns)
    static constexpr G4int kNT_TOFEnergy = 5;     // Neutron energy from TOF (keV)

  private:
    void Book();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
