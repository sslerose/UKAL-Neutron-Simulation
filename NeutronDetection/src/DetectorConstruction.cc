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
/// \file DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class
//
//

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "DetectorSD.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4GeometryManager.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4RunManager.hh"
#include "G4SolidStore.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4VisManager.hh"
#include "G4SDManager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction()
{
  //========================================================================//
  // Set default dimensions based on Feinberg thesis experimental setup
  //========================================================================//
  
  // World volume
  fWorldSize = 2.0 * m;

  // Mu metal holder volume
  fHolderLength = 10.0 * cm;
  fHolderInnerRadius = 5.7 * cm / 2;
  fHolderOuterRadius = 6.0 * cm / 2;
  fHolderOffset = 1.63 * cm / 2;

  // Inner assembly volume (for positioning can and detector inside holder)
  fInnerAssemblyLength = 3.26 * cm;
  fInnerAssemblyRadius = fHolderInnerRadius;

  // Aluminum can volume
  fCanLength = fInnerAssemblyLength;
  fCanInnerRadius = 5.58 * cm / 2;
  fCanOuterRadius = fInnerAssemblyRadius;

  fCanCapThickness = 0.07 * cm;
  fCanCapRadius = fCanInnerRadius;
  fCanCapOffset = -(fCanLength - fCanCapThickness) / 2;

  // Silicon rubber volume
  fRubberThickness = 0.1 * cm;
  fRubberRadius = fCanCapRadius;
  fRubberOffset = fCanCapOffset + (fCanCapThickness + fRubberThickness) / 2;

  // Teflon volume
  fTeflonThickness = 0.025 * cm;
  fTeflonOffset = fRubberOffset + (fRubberThickness + fTeflonThickness) / 2;

  // Detector glass volume
  fDetectorLength = 2.54 * cm;
  fDetectorRadius = 5.08 * cm / 2;
  fDetectorOffset = fTeflonOffset + (fTeflonThickness + fDetectorLength) / 2;

  // Teflon volume (cont.)
  fTeflonRadius = fDetectorRadius;

  // Photomultiplier tube (PMT) volume
  fPMTThickness = 0.25 * cm;
  fPMTRadius = fDetectorRadius;
  fPMTOffset = fDetectorOffset + (fDetectorLength + fPMTThickness) / 2;

  // Detector assembly volume
  fAssemblyLength = fHolderLength + fCanLength / 2;
  fAssemblyWidth = fHolderOuterRadius * 2;
  fAssemblyDistance = 51.0 * cm;  // Distance from target to detector assembly face

  fAssemblyAngle = 0.0 * degree;
  fStartAngle = 0.0 * degree;
  fEndAngle = 360.0 * degree;
  
  DefineMaterials();
  fDetectorMessenger = new DetectorMessenger(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{
  delete fDetectorMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  return ConstructVolumes();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::DefineMaterials()
{
  G4NistManager* nist = G4NistManager::Instance();
  
  // World material - Vacuum
  fWorldMaterial = nist->FindOrBuildMaterial("G4_Galactic");
  
  G4int ncomponents, natoms;
  G4double abundance, density;
  

  //========================================================================//
  // Get/Formulate elements and materials for detector assembly components
  //========================================================================//

  // Create Lithium isotopes
  G4Isotope* Li6 = new G4Isotope("Li6", 3, 6, 6.015122 * g / mole);
  G4Isotope* Li7 = new G4Isotope("Li7", 3, 7, 7.016003 * g / mole);
  
  // Create enriched lithium element (96% Li-6, 4% Li-7)
  G4Element* Li_enriched = new G4Element("Enriched_Lithium", "6Li", ncomponents = 2);
  Li_enriched->AddIsotope(Li6, abundance = 96.0 * perCent);
  Li_enriched->AddIsotope(Li7, abundance = 4.0 * perCent);
  
  // Get other elements
  G4Element* H = nist->FindOrBuildElement("H");     // Hydrogen
  G4Element* C = nist->FindOrBuildElement("C");     // Carbon
  G4Element* O = nist->FindOrBuildElement("O");     // Oxygen
  G4Element* Mg = nist->FindOrBuildElement("Mg");   // Magnesium
  G4Element* Al = nist->FindOrBuildElement("Al");   // Aluminum
  G4Element* Si = nist->FindOrBuildElement("Si");   // Silicon
  G4Element* Ti = nist->FindOrBuildElement("Ti");   // Titanium
  G4Element* Cr = nist->FindOrBuildElement("Cr");   // Chromium
  G4Element* Mn = nist->FindOrBuildElement("Mn");   // Manganese
  G4Element* Fe = nist->FindOrBuildElement("Fe");   // Iron
  G4Element* Ni = nist->FindOrBuildElement("Ni");   // Nickel
  G4Element* Cu = nist->FindOrBuildElement("Cu");   // Copper
  G4Element* Zn = nist->FindOrBuildElement("Zn");   // Zinc
  G4Element* Mo = nist->FindOrBuildElement("Mo");   // Molybdenum
  G4Element* Ce = nist->FindOrBuildElement("Ce");   // Cerium
  
  // Get molecular compounds
  G4Material* SiO2 = nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE");
  G4Material* MgO = nist->FindOrBuildMaterial("G4_MAGNESIUM_OXIDE");
  G4Material* Al2O3 = nist->FindOrBuildMaterial("G4_ALUMINUM_OXIDE");
  G4Material* B2O3 = nist->FindOrBuildMaterial("G4_BORON_OXIDE");
  G4Material* Na2O = nist->FindOrBuildMaterial("G4_SODIUM_MONOXIDE");

  // Formulate other materials
  density = 6.2 * g / cm3;
  G4Material* Ce2O3 = new G4Material("Cerium_Oxide", density, ncomponents = 2);
  Ce2O3->AddElement(Ce, natoms = 2);
  Ce2O3->AddElement(O, natoms = 3);

  // Formulate Li2O with enriched Li
  // Enriched density = natural density * (enriched molar mass / natural molar mass)
  // Natural Li2O density: 2.013 g/cm³
  // Natural Li2O molar mass: 29.88 g/mole
  // Enriched Li atomic mass: 0.96 * 6.015 + 0.04 * 7.016 = 6.055 g/mole
  // Enriched Li2O molar mass: 2 * 6.055 + 15.999 = 28.109 g/mole
  // Enriched Li2O density = 2.013 * (28.109 / 29.88) = 1.8937 g/cm³
  density = 1.8937 * g / cm3;
  G4Material* Li2O = new G4Material("Enriched_Lithium_Oxide", density, ncomponents = 2);
  Li2O->AddElement(Li_enriched, natoms = 2);
  Li2O->AddElement(O, natoms = 1);


  //========================================================================//
  // Create detector components
  //========================================================================//

  // Teflon material
  fTeflonMaterial = nist->FindOrBuildMaterial("G4_TEFLON");

  // Aluminum alloy material (6061-T6)
  // Composition: Al 98.5%, Mg 0.8%, Si 0.4%, Cu 0.15%, Cr 0.05%, Zn 0.05%, Ti 0.05%
  // Density: 2.7 g/cm³
  density = 2.7 * g / cm3;
  fAluminumAlloyMaterial = new G4Material("Aluminum_Alloy_6061_T6", density, ncomponents = 7);

  fAluminumAlloyMaterial->AddElement(Al, 98.5 * perCent);
  fAluminumAlloyMaterial->AddElement(Mg, 0.8 * perCent);
  fAluminumAlloyMaterial->AddElement(Si, 0.4 * perCent);
  fAluminumAlloyMaterial->AddElement(Cu, 0.15 * perCent);
  fAluminumAlloyMaterial->AddElement(Cr, 0.05 * perCent);
  fAluminumAlloyMaterial->AddElement(Zn, 0.05 * perCent);
  fAluminumAlloyMaterial->AddElement(Ti, 0.05 * perCent);

  // Silicon rubber material
  // Composition: H 8.2%, C 32.4%, O 21.6%, Si 37.8%
  // Density: 1.1 g/cm³
  density = 1.1 * g / cm3;
  fSiliconRubberMaterial = new G4Material("Silicon_Rubber", density, ncomponents = 4);

  fSiliconRubberMaterial->AddElement(H, 8.2 * perCent);
  fSiliconRubberMaterial->AddElement(C, 32.4 * perCent);
  fSiliconRubberMaterial->AddElement(O, 21.6 * perCent);
  fSiliconRubberMaterial->AddElement(Si, 37.8 * perCent);

  // Li-6 Glass material based on Feinberg thesis (Table 8)
  // Composition: SiO2 56%, MgO 4%, Ce2O3 4%, Al2O3 18%, 6Li2O 18%
  // Density: 2.6 g/cm³, Li-6 enrichment: 96%
  density = 2.6 * g / cm3;
  fLi6GlassMaterial = new G4Material("Li6Glass", density, ncomponents = 5);

  fLi6GlassMaterial->AddMaterial(SiO2, 56.0 * perCent);
  fLi6GlassMaterial->AddMaterial(MgO, 4.0 * perCent);
  fLi6GlassMaterial->AddMaterial(Ce2O3, 4.0 * perCent);
  fLi6GlassMaterial->AddMaterial(Al2O3, 18.0 * perCent);
  fLi6GlassMaterial->AddMaterial(Li2O, 18.0 * perCent);

  // Borosilicate material
  // Composition: SiO2 81%, B2O3 13%, Na2O 4%, Al2O3 2%
  // Density: 2.2 g/cm³
  density = 2.2 * g / cm3;
  fBorosilicateMaterial = new G4Material("Borosilicate", density, ncomponents = 4);

  fBorosilicateMaterial->AddMaterial(SiO2, 81.0 * perCent);
  fBorosilicateMaterial->AddMaterial(B2O3, 13.0 * perCent);
  fBorosilicateMaterial->AddMaterial(Na2O, 4.0 * perCent);
  fBorosilicateMaterial->AddMaterial(Al2O3, 2.0 * perCent);

  // Mu metal material
  // Composition: C 0.02%, Mn 0.5%, Si 0.35%, Ni 80%, Fe 14.93%, Mo 4.2%
  // Density: 8.75 g/cm³
  density = 8.75 * g / cm3;
  fMuMetalMaterial = new G4Material("Mu_Metal", density, ncomponents = 6);

  fMuMetalMaterial->AddElement(C, 0.02 * perCent);
  fMuMetalMaterial->AddElement(Mn, 0.5 * perCent);
  fMuMetalMaterial->AddElement(Si, 0.35 * perCent);
  fMuMetalMaterial->AddElement(Ni, 80.0 * perCent);
  fMuMetalMaterial->AddElement(Fe, 14.93 * perCent);
  fMuMetalMaterial->AddElement(Mo, 4.2 * perCent);

  // Tape material
  // Composition: SiO2 50% and Si rubber 50%
  // Density: 0.9 g/cm³
  //density = 0.9 * g / cm3;
  //G4Material* tape = new G4Material("Tape", density, ncomponents = 2);

  //tape->AddMaterial(SiO2, 50.0 * perCent);
  //tape->AddMaterial(fSiliconRubberMaterial, 50.0 * perCent);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::ConstructVolumes()
{
  // Cleanup old geometry
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();

  //==========================================================================
  // World Volume
  //==========================================================================
  G4Box* worldBox = new G4Box("World", 
                              fWorldSize / 2, 
                              fWorldSize / 2, 
                              fWorldSize / 2);

  fWorldLV = new G4LogicalVolume(worldBox, 
                                 fWorldMaterial, 
                                 "World");

  fWorldPV = new G4PVPlacement(0,                      // no rotation
                               G4ThreeVector(),        // at (0,0,0)
                               fWorldLV,               // logical volume
                               "World",                // name
                               0,                      // mother volume
                               false,                  // no boolean operation
                               0);                     // copy number

  //==========================================================================
  // Li-6 Glass Detector (cylindrical, based on thesis geometry)
  // Neutrons are generated at origin (copper backplate handled by SimLiT)
  //==========================================================================

  // Create large mother volume containing detector assembly
  // Position mother detector assembly volume in world volume
  // Front face at fAssemblyDistance from origin along z-axis
  G4Box* assemblyBox = new G4Box("AssemblyMother",
                                 fAssemblyWidth / 2,    // Half-width of detector assembly
                                 fAssemblyWidth / 2,
                                 fAssemblyLength / 2);  // Half-length of detector assembly

  fAssemblyLV = new G4LogicalVolume(assemblyBox,
                                    fWorldMaterial,
                                    "AssemblyMother");

  G4double assemblyZ = fAssemblyDistance + fAssemblyLength / 2;
  
  fAssemblyPV = new G4PVPlacement(0,                              // no rotation
                                  G4ThreeVector(0, 0, assemblyZ), // position at assemblyZ
                                  fAssemblyLV,                    // logical volume
                                  "AssemblyMother",               // name
                                  fWorldLV,                       // mother volume
                                  false,                          // no boolean operation
                                  0);                             // copy number


  // Create detector holder (mu metal)
  // Position holder inside assembly volume
  // Front end at fHolderDistance from assembly volume front face
  G4Tubs* holderTube = new G4Tubs("Holder",
                                   fHolderInnerRadius,       // inner radius
                                   fHolderOuterRadius,       // outer radius
                                   fHolderLength / 2,        // half length
                                   fStartAngle,                      // start angle
                                   fEndAngle);          // spanning angle

  fHolderLV = new G4LogicalVolume(holderTube,
                                  fMuMetalMaterial,
                                  "Holder");

  fHolderPV = new G4PVPlacement(0,                          // no rotation
                                G4ThreeVector(0, 0, fHolderOffset), // position
                                fHolderLV,                  // logical volume
                                "Holder",                   // name
                                fAssemblyLV,                // mother volume
                                false,                      // no boolean operation
                                0);                         // copy number


  // Create inner assembly volume (for positioning can and detector inside holder)
  // Position inside assembly volume
  // Front end flush with assembly volume front face
  G4Tubs* innerAssemblyTube = new G4Tubs("InnerAssembly",
                                         0.0,                      // inner radius (solid cylinder)
                                         fInnerAssemblyRadius,     // outer radius
                                         fInnerAssemblyLength / 2, // half length
                                         fStartAngle,                      // start angle
                                         fEndAngle);               // spanning angle

  fInnerAssemblyLV = new G4LogicalVolume(innerAssemblyTube,
                                         fWorldMaterial,
                                         "InnerAssembly");

  G4double innerAssemblyZ = -(fAssemblyLength - fInnerAssemblyLength) / 2;  // position at front of assembly volume

  fInnerAssemblyPV = new G4PVPlacement(0,                                    // no rotation
                                       G4ThreeVector(0, 0, innerAssemblyZ),  // position
                                       fInnerAssemblyLV,                     // logical volume
                                       "InnerAssembly",                      // name
                                       fAssemblyLV,                          // mother volume
                                       false,                                // no boolean operation
                                       0);                                   // copy number


  // Create aluminum can
  // Position can inside inner assembly volume
  // Front end flush with inner assembly volume front face
  G4Tubs* canTube = new G4Tubs("Can",
                                fCanInnerRadius,  // inner radius
                                fCanOuterRadius,  // outer radius
                                fCanLength / 2,   // half length
                                fStartAngle,              // start angle
                                fEndAngle);       // spanning angle
                              
  fCanLV = new G4LogicalVolume(canTube,
                               fAluminumAlloyMaterial,
                               "Can");

  fCanPV = new G4PVPlacement(0,               // no rotation
                             G4ThreeVector(), // position
                             fCanLV,          // logical volume
                             "Can",           // name
                             fInnerAssemblyLV,     // mother volume
                             false,           // no boolean operation
                             0);              // copy number


  // Create can end cap
  // Position inside can volume
  // Cap-off front end of can
  G4Tubs* canCap = new G4Tubs("CanCap",
                              0.0,                  // inner radius (solid cylinder)
                              fCanCapRadius,        // outer radius
                              fCanCapThickness / 2, // half length
                              fStartAngle,                  // start angle
                              fEndAngle);           // spanning angle

  fCanCapLV = new G4LogicalVolume(canCap,
                                  fAluminumAlloyMaterial,
                                  "CanCap");

  fCanCapPV = new G4PVPlacement(0,                                  // no rotation
                                G4ThreeVector(0, 0, fCanCapOffset), // position
                                fCanCapLV,                          // logical volume
                                "CanCap",                           // name
                                fInnerAssemblyLV,                             // mother volume
                                false,                              // no boolean operation
                                0);                                 // copy number
  

  // Create silicon rubber layer
  // Position rubber inside aluminum can
  // Front end flush with inside face of can cap
  G4Tubs* rubberTube = new G4Tubs("Rubber",
                                   0.0,                   // inner radius (solid cylinder)
                                   fRubberRadius,         // outer radius
                                   fRubberThickness / 2,  // half length
                                   fStartAngle,                   // start angle
                                   fEndAngle);            // spanning angle

  fRubberLV = new G4LogicalVolume(rubberTube,
                                  fSiliconRubberMaterial,
                                  "Rubber");

  fRubberPV = new G4PVPlacement(0,                          // no rotation
                                G4ThreeVector(0, 0, fRubberOffset), // position
                                fRubberLV,                  // logical volume
                                "Rubber",                   // name
                                fInnerAssemblyLV,                     // mother volume
                                false,                      // no boolean operation
                                0);                         // copy number


  // Create Teflon layer
  // Position Teflon inside aluminum can
  // Front end flush with rear face of rubber layer
  G4Tubs* teflonTube = new G4Tubs("Teflon",
                                  0.0,                    // inner radius (solid cylinder)
                                  fTeflonRadius,          // outer radius
                                  fTeflonThickness / 2,   // half length
                                  fStartAngle,                    // start angle
                                  fEndAngle);             // spanning angle

  fTeflonLV = new G4LogicalVolume(teflonTube,
                                  fTeflonMaterial,
                                  "Teflon");

  fTeflonPV = new G4PVPlacement(0,                          // no rotation
                                G4ThreeVector(0, 0, fTeflonOffset), // position
                                fTeflonLV,                  // logical volume
                                "Teflon",                   // name
                                fInnerAssemblyLV,                     // mother volume
                                false,                      // no boolean operation
                                0);                         // copy number


  // Create Li-6 glass detector
  // Position detector inside aluminum can
  // Detector center offset by fDetectorOffset from can center
  G4Tubs* detectorTube = new G4Tubs("Detector",
                                    0.0,                  // inner radius (solid cylinder)
                                    fDetectorRadius,      // outer radius
                                    fDetectorLength / 2,  // half length
                                    fStartAngle,                  // start angle
                                    fEndAngle);           // spanning angle

  fDetectorLV = new G4LogicalVolume(detectorTube,
                                    fLi6GlassMaterial,
                                    "Detector");
  
  fDetectorPV = new G4PVPlacement(0,                            // no rotation
                                  G4ThreeVector(0, 0, fDetectorOffset), // position
                                  fDetectorLV,                  // logical volume
                                  "Detector",                   // name
                                  fInnerAssemblyLV,                       // mother volume
                                  false,                        // no boolean operation
                                  0);                           // copy number


  // Create photomultiplier tube (PMT) layer
  // Position PMT inside aluminum can
  // Front end flush with rear face of detector
  G4Tubs* pmtTube = new G4Tubs("PMT",
                               0.0,                 // inner radius (solid cylinder)
                               fPMTRadius,          // outer radius
                               fPMTThickness / 2,   // half length
                               fStartAngle,                 // start angle
                               fEndAngle);          // spanning angle

  fPMTLV = new G4LogicalVolume(pmtTube,
                               fBorosilicateMaterial,
                               "PMT");

  fPMTPV = new G4PVPlacement(0,                         // no rotation
                             G4ThreeVector(0, 0, fPMTOffset), // position
                             fPMTLV,                    // logical volume
                             "PMT",                     // name
                             fInnerAssemblyLV,                    // mother volume
                             false,                     // no boolean operation
                             0);                        // copy number

  PrintParameters();

  // Always return the root volume
  return fWorldPV;
}

void DetectorConstruction::ConstructSDandField()
{
  // Create sensitive detector for Li-6 glass
  DetectorSD* detectorSD = new DetectorSD("/neutrondet/Li6GlassSD", 
                                          "DetectorHitsCollection");
  
  // Register with SD Manager
  G4SDManager::GetSDMpointer()->AddNewDetector(detectorSD);
  
  // Attach to Li-6 glass logical volume
  SetSensitiveDetector(fDetectorLV, detectorSD);

  G4cout << "Sensitive detector attached" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::PrintParameters()
{
  G4cout << "\n========================================" << G4endl;
  G4cout << "Detector Configuration:" << G4endl;
  G4cout << "========================================" << G4endl;
  G4cout << "World size: " << G4BestUnit(fWorldSize, "Length") << G4endl;
  
  G4cout << "\nLi-6 Glass Detector (Cylindrical):" << G4endl;
  G4cout << "  Diameter: " << G4BestUnit(2*fDetectorRadius, "Length") << G4endl;
  G4cout << "  Thickness: " << G4BestUnit(fDetectorLength, "Length") << G4endl;
  G4double detectorZ = fDetectorOffset + fAssemblyDistance + fDetectorLength / 2;
  G4cout << "  Position (center): (0, 0, " << G4BestUnit(detectorZ, "Length") << ")" << G4endl;
  G4double detectorFrontZ = detectorZ - fDetectorLength / 2;
  G4cout << "  Detector face distance from origin: " << G4BestUnit(fAssemblyDistance, "Length") << G4endl;
  G4cout << "  Material: " << fLi6GlassMaterial->GetName() << G4endl;
  G4cout << "  Density: " << fLi6GlassMaterial->GetDensity()/(g/cm3) << " g/cm3" << G4endl;
  G4cout << "\nNote: Neutrons generated at origin (0,0,0)" << G4endl;
  G4cout << "      Copper backplate handled by SimLiT" << G4endl;
  G4cout << "========================================\n" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetDetectorThickness(G4double value)
{
  fDetectorLength = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetDetectorRadius(G4double value)
{
  fDetectorRadius = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetDetectorDistance(G4double value)
{
  fAssemblyDistance = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();

  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    visManager->GeometryHasChanged();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetDetectorAngle(G4double value)
{
  fAssemblyAngle = value;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetSpanningStartAngle(G4double startAngle)
{
  fStartAngle = startAngle;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetSpanningEndAngle(G4double endAngle)
{
  fEndAngle = endAngle;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......