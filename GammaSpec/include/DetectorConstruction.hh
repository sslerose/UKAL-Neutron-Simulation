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
#include "G4SubtractionSolid.hh"
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
    G4Material* MaterialWithSingleIsotope(G4String, G4String, G4double, G4int, G4int);

    // Setters
    void SetDetectorDistance(G4double);
    // void SetDetectorAngle(G4double);
    void SetWorldMaterial(G4String);
    void SetSpanningStartAngle(G4double);
    void SetSpanningEndAngle(G4double);
    void SetAbsorberMaterial(G4String);
    void SetAbsorberThickness(G4double);
    void SetAbsorberRadius(G4double);

    // Getters
    const G4VPhysicalVolume* GetWorld() const { return fWorldPV; }
    // G4LogicalVolume* GetHPGELV(G4int i) const { return (i == 0 || i == 1) ? fHPGELV[i] : nullptr; } // Return the i-th HPGe logical volume
    G4LogicalVolume* GetActiveCrystalLV() const { return fActiveCrystalLV; }
    G4LogicalVolume* GetAbsorberLV() const { return fAbsorberLV; }
    G4LogicalVolume* GetGoldLV() const { return fGoldLV; }

    G4double GetDetectorAngle() const { return fHPGEAngle; }
    G4double GetDetectorDistance() const { return fHPGEDistance; }

    G4double GetSpanningStartAngle() const { return fStartAngle; }
    G4double GetSpanningEndAngle() const { return fEndAngle; }
    G4double GetAbsorberThickness() const { return fAbsorberThickness; }
    G4double GetAbsorberRadius() const { return fAbsorberRadius; }
    G4Material* GetDetectorMaterial() const { return fHPGEMaterial; }
    G4Material* GetAbsorberMaterial() const { return fAbsorberMaterial; }

    void PrintParameters();

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
    G4double fHPGEDistance = 5.0 * cm;
    G4double fHPGEDisplacement = 0.;
    G4double fHPGEAngle = 0. * deg;

    // Materials
    G4Material* fHPGEMaterial = nullptr;
    G4Material* fVacuumMaterial = nullptr;
    G4Material* fAluminumMaterial = nullptr;
    G4Material* fMylarMaterial = nullptr;

    // Misc.
    DetectorMessenger* fDetectorMessenger = nullptr;
    G4RotationMatrix* fRotationMatrix[2] = {nullptr, nullptr}; // Array of rotation matrices for HPGe detector (2 for front and back)
    
    // Volumes
    G4LogicalVolume* fHPGEMotherLV[2] = {nullptr, nullptr}; // Array of logical volumes for HPGe detector (2 for front and back)
    G4LogicalVolume* fInnerStructLV[2] = {nullptr, nullptr};
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

    G4VPhysicalVolume* fHPGEMotherPV[2] = {nullptr, nullptr}; // Array of physical volumes for HPGe detector mother volumes (2 for front and back)

    // G4VPhysicalVolume* fHPGEPV[2] = {nullptr, nullptr}; // Array of physical volumes for HPGe detector (2 for front and back)
    

    //========================================================================//
    // Absorber variables and volumes
    //========================================================================//

    // Gold foil
    G4double fGoldThickness = 0.03 * mm;  // Thickness of gold foil

    // Absorber
    G4double fAbsorberThickness = 0.03 * mm;  // Thickness of absorber
    G4double fAbsorberRadius = 18.0 * mm / 2; // Radius of absorber (cylindrical)

    // Absorber assembly (includes gold foils)
    G4double fStartAngle = 0.0 * deg;
    G4double fEndAngle = 360.0 * deg;

    // Materials
    G4Material* fGoldMaterial = nullptr;
    G4Material* fAbsorberMaterial = nullptr;

    // Solids
    G4Tubs* fAbsorberAssemblyTube = nullptr;
    // G4Tubs* fGoldFrontTube = nullptr;
    // G4Tubs* fGoldBackTube = nullptr;
    G4Tubs* fGoldTube = nullptr;
    G4Tubs* fAbsorberTube = nullptr;

    // Volumes
    G4LogicalVolume* fAbsorberAssemblyLV = nullptr;
    // G4LogicalVolume* fGoldFrontLV = nullptr;
    // G4LogicalVolume* fGoldBackLV = nullptr;
    G4LogicalVolume* fGoldLV = nullptr;
    // G4LogicalVolume* fGoldLV = nullptr;
    G4LogicalVolume* fAbsorberLV = nullptr;
    
    G4VPhysicalVolume* fAbsorberAssemblyPV = nullptr;
    // G4VPhysicalVolume* fGoldFrontPV = nullptr;
    // G4VPhysicalVolume* fGoldBackPV = nullptr;
    G4VPhysicalVolume* fGoldPV[2] = {nullptr, nullptr};
    // G4VPhysicalVolume* fAbsorberPV = nullptr;

  private:
    void DefineMaterials();
    void BuildDetectorStack(G4LogicalVolume* motherLV, G4int detectorID);

    G4VPhysicalVolume* ConstructVolumes();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif