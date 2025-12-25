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
/// \file DetectorConstruction.hh
/// \brief Definition of the DetectorConstruction class
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

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

    // Setters
    void SetDetectorThickness(G4double);
    void SetDetectorRadius(G4double);
    void SetDetectorDistance(G4double);
    void SetDetectorAngle(G4double);
    void SetSpanningStartAngle(G4double);
    void SetSpanningEndAngle(G4double);

    // Getters
    const G4VPhysicalVolume* GetWorld() const { return fWorldPV; }
    const G4VPhysicalVolume* GetDetector() const { return fDetectorPV; }
    
    G4double GetDetectorThickness() const { return fDetectorLength; }
    G4double GetDetectorRadius() const { return fDetectorRadius; }
    G4double GetDetectorDistance() const { return fAssemblyDistance; }
    
    G4Material* GetDetectorMaterial() const { return fLi6GlassMaterial; }

    void PrintParameters();

  private:
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

    // Dimensions
    // World volume
    G4double fWorldSize = 0.;

    // Detector assembly volume
    G4double fAssemblyLength = 0.;
    G4double fAssemblyWidth = 0.;
    G4double fAssemblyDistance = 0.;
    G4double fAssemblyAngle = 0.;
    G4double fStartAngle = 0.;
    G4double fEndAngle = 0.;

    // Mu metal holder volume
    G4double fHolderLength = 0.;
    G4double fHolderInnerRadius = 0.;
    G4double fHolderOuterRadius = 0.;
    G4double fHolderOffset = 0.;

    // Inner assembly volume (for positioning can and detector inside holder)
    G4double fInnerAssemblyLength = 0.;
    G4double fInnerAssemblyRadius = 0.;

    // Aluminum can volume
    G4double fCanLength = 0.;
    G4double fCanInnerRadius = 0.;
    G4double fCanOuterRadius = 0.;
    G4double fCanCapThickness = 0.;
    G4double fCanCapRadius = 0.;
    G4double fCanCapOffset = 0.;

    // Silicon rubber volume
    G4double fRubberThickness = 0.;
    G4double fRubberRadius = 0.;
    G4double fRubberOffset = 0.;

    // Teflon volume
    G4double fTeflonThickness = 0.;
    G4double fTeflonRadius = 0.;
    G4double fTeflonOffset = 0.;

    // Detector glass volume
    G4double fDetectorLength = 0.;
    G4double fDetectorRadius = 0.;
    G4double fDetectorOffset = 0.;

    // Tape volume (irrelevant for current simulations)
    // G4double fTapeThickness = 0.;
    // G4double fTapeRadius = 0.;
    // G4double fTapeDistance = 0.;

    // Photomultiplier tube (PMT) volume
    G4double fPMTThickness = 0.;
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

    DetectorMessenger* fDetectorMessenger = nullptr;

  private:
    void DefineMaterials();
    G4VPhysicalVolume* ConstructVolumes();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif