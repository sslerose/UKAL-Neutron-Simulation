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

#include "globals.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class HistoManager
{
  public:
    HistoManager();
    ~HistoManager() = default;

    //========================================================================//
    // Ntuple column ID constants
    //========================================================================//

    // Column IDs for energy deposition ntuple
    static constexpr G4int kNT_EnergyDep = 0;   // Energy deposited in the detector (MeV)
    static constexpr G4int kNT_EDepWeight = 1;   // Weight associated with the energy deposition (for weighted events)
    static constexpr G4int kNT_EDepTime = 2;        // Global time of the step (us)

    // Column IDs for emitted particle ntuple
    static constexpr G4int kNT_EmissionPID = 0;
    static constexpr G4int kNT_EmEnergy = 1;
    static constexpr G4int kNT_EmWeight = 2;
    static constexpr G4int kNT_EmTime = 3;

    // Column IDs for decay product ntuple
    static constexpr G4int kNT_DecayPID = 0;
    static constexpr G4int kNT_DecayZ = 1;
    static constexpr G4int kNT_DecayA = 2;
    static constexpr G4int kNT_DecayCreatorProcess = 3;
    // static constexpr G4int kNT_DecayKineticEnergy = 4;
    static constexpr G4int kNT_DecayExcitationEnergy = 4;
    static constexpr G4int kNT_DecayWeight = 5;
    static constexpr G4int kNT_DecayIsStable = 6;
    static constexpr G4int kNT_DecayTimeBirth = 7;
    static constexpr G4int kNT_DecayTimeDeath = 8;

  private:
    void Book();
    G4String fFileName = "GammaSpec";  // Default filename
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif
