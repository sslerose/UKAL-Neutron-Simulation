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
constexpr G4double kHolderLength = 10.0 * cm;
constexpr G4double kHolderInnerRadius = 5.7 * cm / 2;
constexpr G4double kHolderOuterRadius = 6.0 * cm / 2;
constexpr G4double kHolderOffset = 1.63 * cm / 2;

// Inner assembly volume
constexpr G4double kInnerAssemblyLength = 3.26 * cm;

// Aluminum can volume
constexpr G4double kCanInnerRadius = 5.58 * cm / 2;
constexpr G4double kCanCapThickness = 0.07 * cm;

// Silicon rubber volume
constexpr G4double kRubberThickness = 0.1 * cm;

// Teflon volume
constexpr G4double kTeflonThickness = 0.025 * cm;

// 6Li glass volume
constexpr G4double kDetectorLength = 2.54 * cm;
constexpr G4double kDetectorRadius = 5.08 * cm / 2;

// Photomultiplier tube (PMT) volume
constexpr G4double kPMTThickness = 0.25 * cm;


//========================================================================//
// Absorber constants
//========================================================================//

// Gold absorber
constexpr G4double kGoldThickness = 0.03 * mm;  // 30 micrometers
constexpr G4double kGoldRadius = 5.0 * mm / 2;


//========================================================================//
// Naming conventions
//========================================================================//

// Sensitive detector and hits collection names
inline G4String kDetectorSDName = "/neutronTOF/Li6GlassSD";
inline G4String kDetectorHCName = "DetectorHitsCollection";


#endif
