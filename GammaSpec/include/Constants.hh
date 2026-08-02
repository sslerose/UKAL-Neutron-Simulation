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
// Detector assembly constants
//========================================================================//

// World volume
constexpr G4double kWorldSize = 2.0 * m;

// Detector assembly volume
// constexpr G4double kHPGELength = 10.0 * cm;
// constexpr G4double kHPGEInnerRadius = 1.5 * cm / 2;
// constexpr G4double kHPGEOuterRadius = 9.0 * cm / 2;
// constexpr G4double kHPGEBoreDepth = 8.5 * cm;

// Cryostat (outermost Al shell)
constexpr G4double kCryoHalfLength        = 4.28   * cm;
constexpr G4double kCryoROut              = 4.400  * cm;
constexpr G4double kCryoRIn               = 4.080  * cm; // = kVac2ROut

// Vacuum gap 2
constexpr G4double kVac2HalfLength        = 4.25   * cm;
constexpr G4double kVac2ROut              = 4.080  * cm;
constexpr G4double kVac2RIn               = 3.635  * cm; // = kDetCupROut

// Detector cup (Al)
constexpr G4double kDetCupHalfLength      = 4.05   * cm;
constexpr G4double kDetCupROut            = 3.635  * cm;
constexpr G4double kDetCupRIn             = 3.537  * cm; // = kVac1ROut

// Vacuum gap 1
constexpr G4double kVac1HalfLength        = 4.00   * cm;
constexpr G4double kVac1ROut              = 3.537  * cm;
constexpr G4double kVac1RIn               = 3.385  * cm; // = kGeCrystalROut

// Outer dead layer (Ge, Li-diffused contact)
constexpr G4double kOuterDeadHalfLength   = 4.00   * cm;
constexpr G4double kOuterDeadROut         = 3.385  * cm;
constexpr G4double kOuterDeadRIn          = 3.3655 * cm; // = kActiveROut

// Active crystal (Ge)
constexpr G4double kActiveHalfLength      = 3.95   * cm;
constexpr G4double kActiveROut            = 3.3655 * cm;
constexpr G4double kActiveRIn             = 0.1805 * cm; // = kInnerDeadROut

// Inner dead layer (Ge, B-implanted contact)
constexpr G4double kInnerDeadHalfLength   = 3.90   * cm;
constexpr G4double kInnerDeadROut         = 0.1805 * cm;
constexpr G4double kInnerDeadRIn          = 0.1005 * cm; // = kCentralROut

// Central contact (vacuum bore)
constexpr G4double kCentralHalfLength     = 3.50   * cm;
constexpr G4double kCentralROut           = 0.1005 * cm;

// Mother volume is flush with the cryostat outer dimensions
constexpr G4double kMotherHalfLength      = kCryoHalfLength;
constexpr G4double kMotherROut            = kCryoROut;

// Distance from the origin to the front face of each detector
constexpr G4double kFaceDistance          = 5.0    * cm;


#endif
