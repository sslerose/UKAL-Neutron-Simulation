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
#include "G4SystemOfUnits.hh"

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
    G4Material* MaterialWithSingleIsotope(G4String, G4String, G4double, G4int, G4int);

    void PrintParameters();

    // Setters
    void SetDetectorDistance(G4int, G4double);
    void SetDetectorAngle(G4int, G4double);
    void SetWorldMaterial(G4String);
    void SetSpanningStartAngle(G4double);
    void SetSpanningEndAngle(G4double);
    void SetAbsorberMaterial(G4String);
    void SetAbsorberThickness(G4double);
    void SetAbsorberRadius(G4double);

    // Getters
    const G4VPhysicalVolume* GetWorld() const { return fWorldPV; }
    G4LogicalVolume* GetActiveCrystalLV() const { return fActiveCrystalLV; }
    G4LogicalVolume* GetAbsorberLV() const { return fAbsorberLV; }
    G4LogicalVolume* GetGoldLV() const { return fGoldLV; }

    G4double GetDetectorAngle(G4int detectorID) const { return fHPGEAngle[detectorID]; }
    G4double GetDetectorDistance(G4int detectorID) const { return fHPGEDistance[detectorID]; }

    G4double GetSpanningStartAngle() const { return fStartAngle; }
    G4double GetSpanningEndAngle() const { return fEndAngle; }
    G4double GetAbsorberThickness() const { return fAbsorberThickness; }
    G4double GetAbsorberRadius() const { return fAbsorberRadius; }
    G4Material* GetDetectorMaterial() const { return fHPGEMaterial; }
    G4Material* GetAbsorberMaterial() const { return fAbsorberMaterial; }

  private:
    //========================================================================//
    // World variables and volumes
    //========================================================================//

    // Size
    G4double fWorldSize = 0.;

    // Material
    G4Material* fWorldMaterial = nullptr;

    // Volumes
    G4LogicalVolume* fWorldLV = nullptr;

    G4VPhysicalVolume* fWorldPV = nullptr;


    //========================================================================//
    // Detector variables and volumes
    //========================================================================//

    // Parameters
    G4double fHPGEDistance[2] = {5.0 * cm, 5.0 * cm};
    G4double fHPGEDisplacement[2] = {0., 0.};
    G4double fHPGEAngle[2] = {0. * deg, 180. * deg};

    // Materials
    G4Material* fHPGEMaterial = nullptr;
    G4Material* fVacuumMaterial = nullptr;
    G4Material* fAluminumMaterial = nullptr;
    G4Material* fMylarMaterial = nullptr;

    // Misc.
    DetectorMessenger* fDetectorMessenger = nullptr;
    G4RotationMatrix* fRotationMatrix[2] = {nullptr, nullptr};

    // Volumes
    G4LogicalVolume* fHPGEMotherLV[2] = {nullptr, nullptr};
    G4LogicalVolume* fInnerStructLV[2] = {nullptr, nullptr};  // Separate inner LVs to avoid overlapping
    G4LogicalVolume* fCryoLV        = nullptr;
    G4LogicalVolume* fCryoCapLV     = nullptr;
    G4LogicalVolume* fAlSheetLV     = nullptr;
    G4LogicalVolume* fMylarSheetLV     = nullptr;
    G4LogicalVolume* fDetCupLV     = nullptr;
    G4LogicalVolume* fDetCupCapLV         = nullptr;
    G4LogicalVolume* fOuterDeadLayerLV  = nullptr;
    G4LogicalVolume* fOuterDeadLayerCapLV  = nullptr;
    G4LogicalVolume* fActiveCrystalLV   = nullptr;
    G4LogicalVolume* fInnerDeadLayerLV  = nullptr;
    G4LogicalVolume* fInnerDeadLayerCapLV  = nullptr;

    G4VPhysicalVolume* fHPGEMotherPV[2] = {nullptr, nullptr};


    //========================================================================//
    // Absorber variables and volumes
    //========================================================================//

    // Gold foil
    G4double fGoldThickness = 0.03 * mm;

    // Absorber
    G4double fAbsorberThickness = 0.03 * mm;
    G4double fAbsorberRadius = 18.0 * mm / 2;

    // Absorber assembly
    G4double fStartAngle = 0.0 * deg; // Initial phi angle
    G4double fEndAngle = 360.0 * deg; // Final phi angle

    // Materials
    G4Material* fGoldMaterial = nullptr;
    G4Material* fAbsorberMaterial = nullptr;

    // Solids
    G4Tubs* fAbsorberAssemblyTube = nullptr;
    G4Tubs* fGoldTube = nullptr;
    G4Tubs* fAbsorberTube = nullptr;

    // Volumes
    G4LogicalVolume* fAbsorberAssemblyLV = nullptr;
    G4LogicalVolume* fGoldLV = nullptr;
    G4LogicalVolume* fAbsorberLV = nullptr;

    G4VPhysicalVolume* fAbsorberAssemblyPV = nullptr;
    G4VPhysicalVolume* fGoldPV[2] = {nullptr, nullptr};

  private:
    void DefineMaterials();
    void BuildDetectorStack(G4LogicalVolume* motherLV, G4LogicalVolume* innerLV, G4int detectorID);

    G4VPhysicalVolume* ConstructVolumes();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif