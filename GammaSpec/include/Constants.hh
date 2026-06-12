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
/// \file Constants.hh
/// \brief Definition of simulation Constants
//

#ifndef Constants_h
#define Constants_h 1

#include "globals.hh"
#include "G4SystemOfUnits.hh"

//========================================================================//
// Detector assembly constants (Ametek GMX45P4-76-A)
//========================================================================//

// World volume
constexpr G4double kWorldSize = 2.0 * m;

// Detector assembly volume
// constexpr G4double kHPGELength = 10.0 * cm;
// constexpr G4double kHPGEInnerRadius = 1.5 * cm / 2;
// constexpr G4double kHPGEOuterRadius = 9.0 * cm / 2;
// constexpr G4double kHPGEBoreDepth = 8.5 * cm;

constexpr G4double kCrystalRadius       = 31.55 * mm;  // A - active crystal radius
constexpr G4double kCrystalLength       = 69.0 * mm;   // B - active crystal length
constexpr G4double kHoleRadius          = 4.4  * mm;   // C - active bore radius
constexpr G4double kHoleDepth           = 60.3 * mm;   // D - active bore depth
constexpr G4double kCupLength           = 105.0 * mm;  // F - cup length
constexpr G4double kGapEncCup           = 4.0  * mm;   // G - encasement-to-cup space
constexpr G4double kCapAlThickness      = 0.03 * mm;   // H - front cap Al layer
constexpr G4double kCapMylarThickness   = 0.03 * mm;   // H - front cap Mylar layer
constexpr G4double kEncThickness        = 1.0  * mm;   // I - encasement wall
constexpr G4double kCupThickness        = 0.8  * mm;   // K - cup wall
constexpr G4double kDeadOuter           = 0.3  * um;   // M - outer (Ge/B) dead layer
constexpr G4double kDeadInner           = 700. * um;   // N - inner (Ge/Li) dead layer
constexpr G4double kBackCapThickness    = 3.0  * mm;   // Cup back cap
constexpr G4double kEncLength           = kEncThickness + kGapEncCup + kCupLength + 10.0 * mm;  // Encasement length
constexpr G4double kGapCupGe            = 2.0  * mm;   // Ge structure-to-cup radial gap


#endif
