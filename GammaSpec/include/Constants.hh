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

// World volume
constexpr G4double kWorldSize = 2.0 * m;

// Detector assembly volume
// constexpr G4double kHPGELength = 10.0 * cm;
// constexpr G4double kHPGEInnerRadius = 1.5 * cm / 2;
// constexpr G4double kHPGEOuterRadius = 9.0 * cm / 2;
// constexpr G4double kHPGEBoreDepth = 8.5 * cm;

//========================================================================//
// Detector component dimensions
//========================================================================//

// Cryostat (outermost Al shell)
constexpr G4double kCryoHalfLength         = 82.5 * mm;
constexpr G4double kCryoRIn                = 37.0 * mm;
constexpr G4double kCryoROut               = kCryoRIn + 1.0 * mm;   // 1 mm thick

constexpr G4double kCryoCapHalfLength      = 0.5 * mm;
constexpr G4double kCryoCapRIn             = 0.0;
constexpr G4double kCryoCapROut            = kCryoRIn;

// Detector cup (Al)
constexpr G4double kDetCupHalfLength       = 52.53 * mm;
constexpr G4double kDetCupRIn              = 33.0 * mm;
constexpr G4double kDetCupROut             = kDetCupRIn + 0.8 * mm; // 0.8 mm thick

constexpr G4double kDetCupCapHalfLength    = 1.5 * mm;
constexpr G4double kDetCupCapRIn           = 4.4 * mm;
constexpr G4double kDetCupCapROut          = kDetCupRIn;

// Al/Mylar sheet
constexpr G4double kAlMylarHalfLength      = 0.03 * mm;
constexpr G4double kAlMylarRIn             = 0.0;
constexpr G4double kAlMylarROut            = kDetCupRIn;

// Active crystal (Ge)
constexpr G4double kActiveHalfLength       = 34.5 * mm;
constexpr G4double kActiveRIn              = 0.0;
constexpr G4double kActiveROut             = 31.55 * mm;

constexpr G4double kActiveBoreHalfLength   = 30.15 * mm;
constexpr G4double kActiveBoreRIn          = 0.0;
constexpr G4double kActiveBoreROut         = 4.4 * mm;

// Outer dead layer (Ge, Li-diffused contact)
constexpr G4double kOuterDeadHalfLength    = kActiveHalfLength;
constexpr G4double kOuterDeadRIn           = kActiveROut;
constexpr G4double kOuterDeadROut          = kActiveROut + 0.3 * um; // 0.3 um thick

constexpr G4double kOuterDeadCapHalfLength = 0.15 * um;
constexpr G4double kOuterDeadCapRIn        = 0.0;
constexpr G4double kOuterDeadCapROut       = kOuterDeadROut;

// Inner dead layer (Ge, B-implanted contact)
constexpr G4double kInnerDeadHalfLength    = kActiveBoreHalfLength;
constexpr G4double kInnerDeadRIn           = 3.7 * mm;
constexpr G4double kInnerDeadROut          = kActiveBoreROut;

constexpr G4double kInnerDeadCapHalfLength = 0.35 * mm;
constexpr G4double kInnerDeadCapRIn        = 0.0;
constexpr G4double kInnerDeadCapROut       = kInnerDeadRIn;

// HPGe mother volume (flush with the cryostat outer dimensions)
constexpr G4double kMotherHalfLength       = kCryoHalfLength;
constexpr G4double kMotherROut             = kCryoROut;

// Inner structure mother volume
constexpr G4double kInnerStructHalfLength  = kDetCupHalfLength;
constexpr G4double kInnerStructRIn         = 0.0;
constexpr G4double kInnerStructROut        = kDetCupROut;

// Distance from the origin to the front face of each detector
// constexpr G4double kFaceDistance          = 5.0    * cm;

//=========================================================================//
// Detector component placement
//=========================================================================//

// Cryostat (mother - fHPGEMotherLV)
constexpr G4double kCryoZPos = 0.0;
constexpr G4double kCryoCapZPos = kMotherHalfLength - kCryoCapHalfLength;

// Inner structure mother volume (mother - fHPGEMotherLV)
constexpr G4double kVacuumGap = 4.0 * mm;
constexpr G4double kInnerStructZPos = kMotherHalfLength - 2 * kCryoCapHalfLength - kVacuumGap - kInnerStructHalfLength;

// Detector cup (mother - fInnerStructLV)
constexpr G4double kDetCupZPos = 0.0;
constexpr G4double kDetCupCapZPos = -kInnerStructHalfLength + kDetCupCapHalfLength;

// Al/Mylar sheet (mother - fInnerStructLV)
constexpr G4double kAlSheetZPos = kInnerStructHalfLength - kAlMylarHalfLength / 2;
constexpr G4double kMylarSheetZPos = kInnerStructHalfLength - 3 * kAlMylarHalfLength / 2;

// Active crystal (mother - fInnerStructLV)
constexpr G4double kActiveZPos = kInnerStructHalfLength - 2 * kAlMylarHalfLength - 2 * kOuterDeadCapHalfLength - kActiveHalfLength;

// Outer dead layer (mother - fInnerStructLV)
constexpr G4double kOuterDeadZPos = kActiveZPos;
constexpr G4double kOuterDeadCapZPos = kActiveZPos + kActiveHalfLength + kOuterDeadCapHalfLength;

// Inner dead layer (mother - fInnerStructLV)
constexpr G4double kInnerDeadZPos = kActiveZPos - kActiveHalfLength + kInnerDeadHalfLength;
constexpr G4double kInnerDeadCapZPos = kInnerDeadZPos + kInnerDeadHalfLength - kInnerDeadCapHalfLength;

#endif
