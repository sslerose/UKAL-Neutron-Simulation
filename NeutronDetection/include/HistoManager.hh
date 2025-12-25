//
// ********************************************************************
// * License and Disclaimer                                           *
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
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file HistoManager.hh
/// \brief Definition of the HistoManager class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

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
    static constexpr G4int kH_NeutronEnergy = 0;   // SimLiT neutron energy (keV)
    static constexpr G4int kH_NeutronTheta = 1;    // SimLiT neutron angle (deg)
    static constexpr G4int kH_nCapture = 2;       // Number of capture events
    static constexpr G4int kH_TOF = 3;             // Time of flight - capture time (ns)
    static constexpr G4int kH_nHits = 4;     // Number of neutrons hits per event
    static constexpr G4int kH_TOFEnergy = 5;       // Neutron energy from TOF (keV)

    static constexpr G4int kH_totTrackLen = 10;    // Total neutron track length (mm)
    static constexpr G4int kH_totCollisions = 11;   // Total neutron collisions
    static constexpr G4int kH_trackLenNonThermal = 12;    // Total neutron track length (mm)
    static constexpr G4int kH_trackLenThermal = 13;      // Total neutron track length (mm)

    //========================================================================//
    // Ntuple column ID constants
    //========================================================================//
    static constexpr G4int kNT_NeutronEnergy = 0;
    static constexpr G4int kNT_NeutronTheta = 1;
    // static constexpr G4int kNT_TotalEdep = 2;
    static constexpr G4int kNT_TOF = 2;
    static constexpr G4int kNT_CaptureFlag = 3;

  private:
    void Book();
    G4String fFileName = "NeutronDetection";
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
