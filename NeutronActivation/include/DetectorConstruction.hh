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
/// \file DetectorConstruction.hh
/// \brief Definition of the DetectorConstruction class
//

#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"

#include "G4Tubs.hh"
#include "G4RotationMatrix.hh"
#include "globals.hh"
#include "Constants.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;
class DetectorMessenger;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

  public:
    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;
    G4Material* MaterialWithSingleIsotope(G4String, G4String, G4double, G4int, G4int);

    // Setters
    void SetDetectorDistance(G4double);
    void SetDetectorAngle(G4double);
    void SetSpanningStartAngle(G4double);
    void SetSpanningEndAngle(G4double);
    void SetWorldMaterial(G4String);
    void SetAbsorberMaterial(G4String);
    void SetAbsorberThickness(G4double);
    void SetAbsorberRadius(G4double);
    void SetAbsorberDistance(G4double);

    // Getters
    const G4VPhysicalVolume* GetWorld() const { return fWorldPV; }
    const G4VPhysicalVolume* GetDetector() const { return fDetectorPV; }
    
    G4double GetDetectorAngle() const { return fAssemblyAngle; }
    G4double GetDetectorDistance() const { return fDetectorDistance; }

    G4double GetSpanningStartAngle() const { return fStartAngle; }
    G4double GetSpanningEndAngle() const { return fEndAngle; }
    G4double GetAbsorberDistance() const { return fAbsorberAssemblyDistance; }
    G4double GetAbsorberThickness() const { return fAbsorberThickness; }
    G4double GetAbsorberRadius() const { return fAbsorberRadius; }
    G4Material* GetDetectorMaterial() const { return fLi6GlassMaterial; }
    G4Material* GetAbsorberMaterial() const { return fAbsorberMaterial; }

    void PrintParameters();

  private:
    //========================================================================//
    // Detector assembly volumes
    //========================================================================//

    // Solids
    G4Tubs* fHolderTube = nullptr;
    G4Tubs* fCanTube = nullptr;
    G4Tubs* fCanCapTube = nullptr;
    G4Tubs* fRubberTube = nullptr;
    G4Tubs* fTeflonTube = nullptr;
    G4Tubs* fDetectorTube = nullptr;
    G4Tubs* fPMTTube = nullptr;
    
    // Physical volumes
    G4VPhysicalVolume* fWorldPV = nullptr;
    G4VPhysicalVolume* fAssemblyPV = nullptr;
    G4VPhysicalVolume* fHolderPV = nullptr;
    G4VPhysicalVolume* fInnerAssemblyPV = nullptr;
    G4VPhysicalVolume* fCanPV = nullptr;
    G4VPhysicalVolume* fCanCapPV = nullptr;
    G4VPhysicalVolume* fRubberPV = nullptr;
    G4VPhysicalVolume* fTeflonPV = nullptr;
    G4VPhysicalVolume* fDetectorPV = nullptr;
    G4VPhysicalVolume* fPMTPV = nullptr;
    
    // Logical volumes
    G4LogicalVolume* fWorldLV = nullptr;
    G4LogicalVolume* fAssemblyLV = nullptr;
    G4LogicalVolume* fHolderLV = nullptr;
    G4LogicalVolume* fInnerAssemblyLV = nullptr;
    G4LogicalVolume* fCanLV = nullptr;
    G4LogicalVolume* fCanCapLV = nullptr;
    G4LogicalVolume* fRubberLV = nullptr;
    G4LogicalVolume* fTeflonLV = nullptr;
    G4LogicalVolume* fDetectorLV = nullptr;
    G4LogicalVolume* fPMTLV = nullptr;


    //========================================================================//
    // Detector assembly parameters
    //========================================================================//

    // World volume
    G4double fWorldSize = 0.;

    // Detector assembly volume
    G4double fAssemblyLength = 0.;
    G4double fAssemblyWidth = 0.;
    G4double fAssemblyDistance = 0.;
    G4double fAssemblyDisplacement = 0.;
    G4double fAssemblyAngle = 0. * deg;
    G4double fStartAngle = 0.0 * deg;
    G4double fEndAngle = 360.0 * deg;

    // Inner assembly volume (for positioning can and detector inside holder)
    G4double fInnerAssemblyLength = 0.;
    G4double fInnerAssemblyRadius = 0.;

    // Aluminum can volume
    G4double fCanLength = 0.;
    G4double fCanOuterRadius = 0.;
    G4double fCanCapRadius = 0.;
    G4double fCanCapOffset = 0.;

    // Silicon rubber volume
    G4double fRubberRadius = 0.;
    G4double fRubberOffset = 0.;

    // Teflon volume
    G4double fTeflonRadius = 0.;
    G4double fTeflonOffset = 0.;

    // Detector glass volume
    G4double fDetectorOffset = 0.;
    G4double fDetectorDistance = 50.0 * cm;

    // Tape volume (irrelevant for current simulations)
    // G4double fTapeThickness = 0.;
    // G4double fTapeRadius = 0.;
    // G4double fTapeDistance = 0.;

    // Photomultiplier tube (PMT) volume
    G4double fPMTRadius = 0.;
    G4double fPMTOffset = 0.;

    // Materials
    G4Material* fWorldMaterial = nullptr;
    G4Material* fTeflonMaterial = nullptr;
    G4Material* fAluminumAlloyMaterial = nullptr;
    G4Material* fSiliconRubberMaterial = nullptr;
    G4Material* fLi6GlassMaterial = nullptr;
    G4Material* fBorosilicateMaterial = nullptr;
    G4Material* fMuMetalMaterial = nullptr;

    // Misc.
    DetectorMessenger* fDetectorMessenger = nullptr;
    G4RotationMatrix* fRotationMatrix = nullptr;


    //========================================================================//
    // Absorber volumes
    //========================================================================//

    // Solids
    G4Tubs* fAbsorberAssemblyTube = nullptr;
    G4Tubs* fGoldFrontTube = nullptr;
    G4Tubs* fGoldBackTube = nullptr;
    G4Tubs* fAbsorberTube = nullptr;
    
    // Physical volumes
    G4VPhysicalVolume* fAbsorberAssemblyPV = nullptr;
    G4VPhysicalVolume* fGoldFrontPV = nullptr;
    G4VPhysicalVolume* fGoldBackPV = nullptr;
    G4VPhysicalVolume* fAbsorberPV = nullptr;
    
    // Logical volumes
    G4LogicalVolume* fAbsorberAssemblyLV = nullptr;
    G4LogicalVolume* fGoldFrontLV = nullptr;
    G4LogicalVolume* fGoldBackLV = nullptr;
    G4LogicalVolume* fAbsorberLV = nullptr;


    //========================================================================//
    // Absorber parameters
    //========================================================================//

    // Gold foil
    G4double fGoldThickness = 0.03 * mm;  // Thickness of gold foil

    // Absorber
    G4double fAbsorberThickness = 0.03 * mm;  // Thickness of absorber
    G4double fAbsorberRadius = 18.0 * mm / 2; // Radius of absorber (cylindrical)

    // Absorber assembly (includes gold foils)
    G4double fAbsorberAssemblyDistance = 5.0 * mm;  // Distance from target to absorber assembly face

    // Materials
    G4Material* fGoldMaterial = nullptr;
    G4Material* fAbsorberMaterial = nullptr;


  private:
    void DefineMaterials();
    G4VPhysicalVolume* ConstructVolumes();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif