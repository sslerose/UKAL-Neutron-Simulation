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
/// \file DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class
//

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "DetectorSD.hh"
#include "Constants.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4RotationMatrix.hh"
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
#include "G4UImanager.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::DetectorConstruction()
{
  //========================================================================//
  // Math for detector assembly dimensions and placement
  //========================================================================//

  // Aluminum can volume
  fCanCapOffset = -(kInnerAssemblyLength - kCanCapThickness) / 2;

  // Silicon rubber volume
  fRubberOffset = fCanCapOffset + (kCanCapThickness + kRubberThickness) / 2;

  // Teflon volume
  fTeflonOffset = fRubberOffset + (kRubberThickness + kTeflonThickness) / 2;

  // Detector glass volume
  fDetectorOffset = fTeflonOffset + (kTeflonThickness + kDetectorLength) / 2;

  // Photomultiplier tube (PMT) volume
  fPMTOffset = fDetectorOffset + (kDetectorLength + kPMTThickness) / 2;

  // Detector assembly volume
  fAssemblyLength = kHolderLength + kInnerAssemblyLength / 2;

  // Detector placement
  fAssemblyDistance = fDetectorDistance - kCanCapThickness - kRubberThickness - kTeflonThickness;  // Dist. from target to assembly face
  fAssemblyDisplacement = fAssemblyDistance + fAssemblyLength / 2;
  fRotationMatrix = new G4RotationMatrix();
  fRotationMatrix->rotateY(fAssemblyAngle); // Rotation of assembly around Y axis
  
  // Initialize materials and detector messenger
  DefineMaterials();
  fDetectorMessenger = new DetectorMessenger(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

DetectorConstruction::~DetectorConstruction()
{
  delete fRotationMatrix;
  delete fDetectorMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  return ConstructVolumes();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Define materials using NIST database and custom formulations
//------------------------------------------------------------------------//
void DetectorConstruction::DefineMaterials()
{
  // Obtain NIST material manager
  G4NistManager* nist = G4NistManager::Instance();
  
  // World material - Vacuum
  fWorldMaterial = nist->FindOrBuildMaterial("G4_Galactic");
  
  // Instance variables for material formulation
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

  // Li-6 Glass material
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
  // density = 0.9 * g / cm3;
  // G4Material* tape = new G4Material("Tape", density, ncomponents = 2);

  // tape->AddMaterial(SiO2, 50.0 * perCent);
  // tape->AddMaterial(fSiliconRubberMaterial, 50.0 * perCent);


  //========================================================================//
  // Create absorber components
  //========================================================================//

  // Gold foil material
  fGoldMaterial = nist->FindOrBuildMaterial("G4_Au");

  // Absorber material
  fAbsorberMaterial = nist->FindOrBuildMaterial("G4_Au");  // To be replaced with actual absorber material by macro
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Construct world and detector assembly volumes
//------------------------------------------------------------------------//
G4VPhysicalVolume* DetectorConstruction::ConstructVolumes()
{
  // Cleanup old geometry
  G4GeometryManager::GetInstance()->OpenGeometry();
  G4PhysicalVolumeStore::GetInstance()->Clean();
  G4LogicalVolumeStore::GetInstance()->Clean();
  G4SolidStore::GetInstance()->Clean();

  //========================================================================//
  // World Volume
  //========================================================================//

  G4Box* worldBox = new G4Box("World", 
                              kWorldSize / 2, 
                              kWorldSize / 2, 
                              kWorldSize / 2);

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


  //========================================================================//
  // Detector assembly geometry (cylindrical)
  // Particles generated at origin
  //========================================================================//

  // Create large mother volume containing detector assembly
  // Position mother detector assembly volume within world volume
  // Front face at fAssemblyDistance from origin along z-axis
  G4Box* assemblyBox = new G4Box("AssemblyMother",
                                 kHolderOuterRadius,    // Half-width of detector assembly
                                 kHolderOuterRadius,
                                 fAssemblyLength / 2);  // Half-length of detector assembly

  fAssemblyLV = new G4LogicalVolume(assemblyBox,
                                    fWorldMaterial,
                                    "AssemblyMother");

  // Calculate assembly position components
  G4double assemblyX = -fAssemblyDisplacement * std::sin(fAssemblyAngle);
  G4double assemblyZ = fAssemblyDisplacement * std::cos(fAssemblyAngle);

  fAssemblyPV = new G4PVPlacement(fRotationMatrix,                // rotation matrix
                                  G4ThreeVector(assemblyX, 0, assemblyZ), // position at assemblyZ
                                  fAssemblyLV,                    // logical volume
                                  "AssemblyMother",               // name
                                  fWorldLV,                       // mother volume
                                  false,                          // no boolean operation
                                  0);                             // copy number


  // Create detector holder (mu metal)
  // Position holder inside assembly volume
  // Front end at fHolderDistance from assembly volume front face
  fHolderTube = new G4Tubs("Holder",
                            kHolderInnerRadius,       // inner radius
                            kHolderOuterRadius,       // outer radius
                            kHolderLength / 2,        // half length
                            0. * deg,                 // initial spanning angle
                            360. * deg);              // final spanning angle

  fHolderLV = new G4LogicalVolume(fHolderTube,
                                  fMuMetalMaterial,
                                  "Holder");

  fHolderPV = new G4PVPlacement(0,                          // no rotation
                                G4ThreeVector(0, 0, kHolderOffset), // position
                                fHolderLV,                  // logical volume
                                "Holder",                   // name
                                fAssemblyLV,                // mother volume
                                false,                      // no boolean operation
                                0);                         // copy number


  // Create inner assembly volume (for positioning can and detector inside holder)
  // Position inside assembly volume
  // Front end flush with assembly volume front face
  G4Tubs* innerAssemblyTube = new G4Tubs("InnerAssembly",
                                         0.0,                       // inner radius (solid cylinder)
                                         kHolderInnerRadius,        // outer radius
                                         kInnerAssemblyLength / 2,  // half length
                                         0. * deg,                  // initial spanning angle
                                         360. * deg);               // final spanning angle

  fInnerAssemblyLV = new G4LogicalVolume(innerAssemblyTube,
                                         fWorldMaterial,
                                         "InnerAssembly");

  G4double innerAssemblyZ = -(fAssemblyLength - kInnerAssemblyLength) / 2;  // position at front of assembly volume

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
  fCanTube = new G4Tubs("Can",
                        kCanInnerRadius,          // inner radius
                        kHolderInnerRadius,       // outer radius
                        kInnerAssemblyLength / 2, // half length
                        0. * deg,                 // initial spanning angle
                        360. * deg);              // final spanning angle
                              
  fCanLV = new G4LogicalVolume(fCanTube,
                               fAluminumAlloyMaterial,
                               "Can");

  fCanPV = new G4PVPlacement(0,                 // no rotation
                             G4ThreeVector(),   // position
                             fCanLV,            // logical volume
                             "Can",             // name
                             fInnerAssemblyLV,  // mother volume
                             false,             // no boolean operation
                             0);                // copy number


  // Create can end cap
  // Position inside can volume
  // Cap-off front end of can (flush with inner assembly volume front face)
  fCanCapTube = new G4Tubs("CanCap",
                            0.0,                  // inner radius (solid cylinder)
                            kCanInnerRadius,      // outer radius
                            kCanCapThickness / 2, // half length
                            0. * deg,             // initial spanning angle
                            360. * deg);          // final spanning angle

  fCanCapLV = new G4LogicalVolume(fCanCapTube,
                                  fAluminumAlloyMaterial,
                                  "CanCap");

  fCanCapPV = new G4PVPlacement(0,                                  // no rotation
                                G4ThreeVector(0, 0, fCanCapOffset), // position
                                fCanCapLV,                          // logical volume
                                "CanCap",                           // name
                                fInnerAssemblyLV,                   // mother volume
                                false,                              // no boolean operation
                                0);                                 // copy number
  

  // Create silicon rubber layer
  // Position rubber inside aluminum can
  // Front end flush with inside face of can cap
  fRubberTube = new G4Tubs("Rubber",
                            0.0,                  // inner radius (solid cylinder)
                            kCanInnerRadius,      // outer radius
                            kRubberThickness / 2, // half length
                            0. * deg,             // initial spanning angle
                            360. * deg);          // final spanning angle

  fRubberLV = new G4LogicalVolume(fRubberTube,
                                  fSiliconRubberMaterial,
                                  "Rubber");

  fRubberPV = new G4PVPlacement(0,                                  // no rotation
                                G4ThreeVector(0, 0, fRubberOffset), // position
                                fRubberLV,                          // logical volume
                                "Rubber",                           // name
                                fInnerAssemblyLV,                   // mother volume
                                false,                              // no boolean operation
                                0);                                 // copy number


  // Create Teflon layer
  // Position Teflon inside aluminum can
  // Front end flush with inside face of rubber layer
  fTeflonTube = new G4Tubs("Teflon",
                            0.0,                  // inner radius (solid cylinder)
                            kDetectorRadius,      // outer radius
                            kTeflonThickness / 2, // half length
                            0. * deg,             // initial spanning angle
                            360. * deg);          // final spanning angle

  fTeflonLV = new G4LogicalVolume(fTeflonTube,
                                  fTeflonMaterial,
                                  "Teflon");

  fTeflonPV = new G4PVPlacement(0,                                  // no rotation
                                G4ThreeVector(0, 0, fTeflonOffset), // position
                                fTeflonLV,                          // logical volume
                                "Teflon",                           // name
                                fInnerAssemblyLV,                   // mother volume
                                false,                              // no boolean operation
                                0);                                 // copy number


  // Create Li-6 glass detector
  // Position detector inside aluminum can
  // Front end flush with inside face of Teflon layer
  fDetectorTube = new G4Tubs("Detector",
                              0.0,                  // inner radius (solid cylinder)
                              kDetectorRadius,      // outer radius
                              kDetectorLength / 2,  // half length
                              0. * deg,             // initial spanning angle
                              360. * deg);          // final spanning angle

  fDetectorLV = new G4LogicalVolume(fDetectorTube,
                                    fLi6GlassMaterial,
                                    "Detector");
  
  fDetectorPV = new G4PVPlacement(0,                                    // no rotation
                                  G4ThreeVector(0, 0, fDetectorOffset), // position
                                  fDetectorLV,                          // logical volume
                                  "Detector",                           // name
                                  fInnerAssemblyLV,                     // mother volume
                                  false,                                // no boolean operation
                                  0);                                   // copy number


  // Create photomultiplier tube (PMT) layer
  // Position PMT inside aluminum can
  // Front end flush with inside face of detector
  fPMTTube = new G4Tubs("PMT",
                        0.0,                // inner radius (solid cylinder)
                        kDetectorRadius,    // outer radius
                        kPMTThickness / 2,  // half length
                        0. * deg,           // initial spanning angle
                        360. * deg);        // final spanning angle

  fPMTLV = new G4LogicalVolume(fPMTTube,
                               fBorosilicateMaterial,
                               "PMT");

  fPMTPV = new G4PVPlacement(0,                               // no rotation
                             G4ThreeVector(0, 0, fPMTOffset), // position
                             fPMTLV,                          // logical volume
                             "PMT",                           // name
                             fInnerAssemblyLV,                // mother volume
                             false,                           // no boolean operation
                             0);                              // copy number



  //========================================================================//
  // Absorber geometry (cylindrical)
  //========================================================================//

  // Absorber assembly volume

  G4double absorberAssemblyThickness = fAbsorberThickness + 2 * fGoldThickness; // Length of absorber assembly (absorber + 2 gold foils)

  // Absorber assembly placement
  fAbsorberAssemblyDisplacement = fAbsorberAssemblyDistance + absorberAssemblyThickness / 2; // Displacement of absorber assembly from origin (front face at fAbsorberAssemblyDistance)

  fAbsorberAssemblyTube = new G4Tubs("AbsorberAssembly",
                                            0.0,                            // inner radius (solid cylinder)
                                            fAbsorberRadius,                // outer radius
                                            absorberAssemblyThickness / 2,  // half length
                                            fStartAngle,                    // initial spanning angle
                                            fEndAngle);                     // final spanning angle

  fAbsorberAssemblyLV = new G4LogicalVolume(fAbsorberAssemblyTube,
                                            fWorldMaterial,
                                            "AbsorberAssembly");

  fAbsorberAssemblyPV = new G4PVPlacement(0,                                      // no rotation
                                          G4ThreeVector(0, 0, fAbsorberAssemblyDisplacement), // position
                                          fAbsorberAssemblyLV,                    // logical volume
                                          "AbsorberAssembly",                     // name
                                          fWorldLV,                               // mother volume
                                          false,                                  // no boolean operation
                                          0);                                     // copy number


  // Gold foil (front)

  G4double goldDisplacement = (fAbsorberThickness + fGoldThickness) / 2; // Displacement of gold foil from absorber assembly center

  fGoldFrontTube = new G4Tubs("GoldFrontFoil",
                              0.0,                // inner radius (solid cylinder)
                              fAbsorberRadius,    // outer radius
                              fGoldThickness / 2, // half length
                              fStartAngle,        // initial spanning angle
                              fEndAngle);         // final spanning angle

  fGoldFrontLV = new G4LogicalVolume(fGoldFrontTube,
                                     fGoldMaterial,
                                     "GoldFrontFoil");

  fGoldFrontPV = new G4PVPlacement(0,                                       // no rotation
                                   G4ThreeVector(0, 0, -goldDisplacement),  // position
                                   fGoldFrontLV,                            // logical volume
                                   "GoldFrontFoil",                         // name
                                   fAbsorberAssemblyLV,                     // mother volume
                                   false,                                   // no boolean operation
                                   0);                                      // copy number


  // Gold foil (back)

  fGoldBackTube = new G4Tubs("GoldBackFoil",
                             0.0,                 // inner radius (solid cylinder)
                             fAbsorberRadius,     // outer radius
                             fGoldThickness / 2,  // half length
                             fStartAngle,         // initial spanning angle
                             fEndAngle);          // final spanning angle

  fGoldBackLV = new G4LogicalVolume(fGoldBackTube,
                                    fGoldMaterial,
                                    "GoldBackFoil");

  fGoldBackPV = new G4PVPlacement(0,                                      // no rotation
                                  G4ThreeVector(0, 0, goldDisplacement), // position
                                  fGoldBackLV,                            // logical volume
                                  "GoldBackFoil",                         // name
                                  fAbsorberAssemblyLV,                    // mother volume
                                  false,                                  // no boolean operation
                                  0);                                     // copy number


  // Absorber

  fAbsorberTube = new G4Tubs("Absorber",
                             0.0,                    // inner radius (solid cylinder)
                             fAbsorberRadius,        // outer radius
                             fAbsorberThickness / 2, // half length
                             fStartAngle,            // initial spanning angle
                             fEndAngle);             // final spanning angle

  fAbsorberLV = new G4LogicalVolume(fAbsorberTube,
                                    fAbsorberMaterial,
                                    "Absorber");

  fAbsorberPV = new G4PVPlacement(0,                      // no rotation
                                  G4ThreeVector(0, 0, 0), // position
                                  fAbsorberLV,            // logical volume
                                  "Absorber",             // name
                                  fAbsorberAssemblyLV,    // mother volume
                                  false,                  // no boolean operation
                                  0);                     // copy number

  // Return the root volume
  return fWorldPV;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Create and attach sensitive detectors (SD) to detector volumes
//------------------------------------------------------------------------//
void DetectorConstruction::ConstructSDandField()
{
  // Create SD manager for Li-6 glass detector
  G4SDManager* sdManager = G4SDManager::GetSDMpointer();
  
  // Check if SD is already attached
  DetectorSD* detectorSD = static_cast<DetectorSD*>(sdManager->FindSensitiveDetector(kDetectorSDName, false));
  
  if (!detectorSD) {
    // SD does not exist - create new
    // SD Name: kDetectorSDName
    // Hits collection name: kDetectorHCName
    detectorSD = new DetectorSD(kDetectorSDName, kDetectorHCName);

    // Register with SD Manager
    sdManager->AddNewDetector(detectorSD);
    G4cout << "Created new sensitive detector: " << kDetectorSDName << G4endl;
  }
  else {
    G4cout << "Using existing sensitive detector: " << kDetectorSDName << G4endl;
  }
  
  // Attach SD to the detector logical volume
  SetSensitiveDetector(fDetectorLV, detectorSD);
  
  G4cout << "Sensitive detector attached to Li-6 glass volume" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set detector assembly distance from origin
//------------------------------------------------------------------------//
void DetectorConstruction::SetDetectorDistance(G4double value)
{
  // Check that geometry has been constructed
  if (!fAssemblyPV) {
    G4cerr << "Detector not yet constructed." << G4endl;
    return;
  }

  // Calculate new assembly coordinates accounting for assembly angle
  fDetectorDistance = value;  // Distance from target to detector face
  fAssemblyDistance = fDetectorDistance - kCanCapThickness - kRubberThickness - kTeflonThickness;  // Distance from target to assembly face

  fAssemblyDisplacement = fAssemblyDistance + fAssemblyLength / 2;
  G4double assemblyX = -fAssemblyDisplacement * fRotationMatrix->xz();
  G4double assemblyZ = fAssemblyDisplacement * fRotationMatrix->xx();

  // Update assembly position
  fAssemblyPV->SetTranslation(G4ThreeVector(assemblyX, 0, assemblyZ));

  G4cout << "New glass detector distance to source: " << G4BestUnit(fDetectorDistance, "Length") << G4endl;
  G4cout << "New detector assembly distance to source: " << G4BestUnit(fAssemblyDistance, "Length") << G4endl;

  // Notify run manager of geometry modification
  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization (if it is active)
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    // G4UImanager::GetUIpointer()->ApplyCommand("/run/reinitializeGeometry");
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set detector assembly angle about y-axis
//------------------------------------------------------------------------//
void DetectorConstruction::SetDetectorAngle(G4double value)
{
  // Check that geometry has been constructed
  if (!fAssemblyPV) {
    G4cerr << "Detector not yet constructed." << G4endl;
    return;
  }

  // Create rotation matrix and calculate new assembly position
  fAssemblyAngle = -value;  // Negative to restore right-handed rotation about y-axis
  *fRotationMatrix = G4RotationMatrix();
  fRotationMatrix->rotateY(fAssemblyAngle);

  G4double assemblyX = -fAssemblyDisplacement * fRotationMatrix->xz();
  G4double assemblyZ = fAssemblyDisplacement * fRotationMatrix->xx();

  // Update assembly rotation and position
  fAssemblyPV->SetRotation(fRotationMatrix);
  fAssemblyPV->SetTranslation(G4ThreeVector(assemblyX, 0, assemblyZ));

  G4cout << "New detector angle: " << -fAssemblyAngle / degree << " degrees" << G4endl;

  // Notify run manager of geometry modification
  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization (if it is active)
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    // G4UImanager::GetUIpointer()->ApplyCommand("/run/reinitializeGeometry");
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set detector assembly angle about y-axis
//------------------------------------------------------------------------//
void DetectorConstruction::SetWorldMaterial(G4String value)
{
  // Check that geometry has been constructed
  if (!fAssemblyPV) {
    G4cerr << "Detector not yet constructed." << G4endl;
    return;
  }

  // Obtain NIST material manager
  G4NistManager* nist = G4NistManager::Instance();
  
  // Material holder
  G4Material* material = nullptr;

  if (value == "vacuum") {
    material = nist->FindOrBuildMaterial("G4_Galactic");
  }
  else if (value == "air") {
    material = nist->FindOrBuildMaterial("G4_AIR");
  }
  else {
    G4cerr << "Invalid world material selection." << G4endl;
    return;
  }

  fWorldLV->SetMaterial(material);
  fAssemblyLV->SetMaterial(material);
  fInnerAssemblyLV->SetMaterial(material);
  fAbsorberAssemblyLV->SetMaterial(material);

  // Create rotation matrix and calculate new assembly position
  fWorldMaterial = material;

  G4cout << "New world material: " << fWorldMaterial->GetName() << G4endl;

  // Notify run manager of geometry modification
  G4RunManager::GetRunManager()->PhysicsHasBeenModified();

  // Update visualization (if it is active)
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Create material with single isotope
//------------------------------------------------------------------------//
G4Material* DetectorConstruction::MaterialWithSingleIsotope(G4String name, G4String symbol,
                                                            G4double density, G4int Z, G4int A)
{
  // Define isotope material
  G4int ncomponents;
  G4double abundance, massfraction;

  G4Isotope* isotope = new G4Isotope(symbol, Z, A);

  G4Element* element = new G4Element(name, symbol, ncomponents = 1);
  element->AddIsotope(isotope, abundance = 100. * perCent);

  G4Material* material = new G4Material(name, density, ncomponents = 1);
  material->AddElement(element, massfraction = 100. * perCent);

  return material;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set absorber material
//------------------------------------------------------------------------//
void DetectorConstruction::SetAbsorberMaterial(G4String value)
{
  G4Material* material = G4NistManager::Instance()->FindOrBuildMaterial(value);

  // Check that geometry has been constructed
  if (!fAbsorberAssemblyPV) {
    G4cerr << "Absorber not yet constructed." << G4endl;
    return;
  }

  if (material) {
    // Update absorber material
    fAbsorberLV->SetMaterial(material);
    fAbsorberMaterial = material;

    G4cout << "New absorber material: " << fAbsorberMaterial->GetName() << G4endl;

    // Notify run manager of geometry modification
    G4RunManager::GetRunManager()->PhysicsHasBeenModified();

    // Update visualization if it is active
    G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
    if (visManager) {
      // Visualization is active - trigger an update
      G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
    }
  }
  else {
    G4cerr << "\n--> warning from DetectorConstruction::SetMaterial : " << value
           << " not found" << G4endl;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set absorber thickness
//------------------------------------------------------------------------//
void DetectorConstruction::SetAbsorberThickness(G4double thickness)
{
  // Check that geometry has been constructed
  if (!fAbsorberAssemblyPV) {
    G4cerr << "Absorber not yet constructed." << G4endl;
    return;
  }

  fAbsorberThickness = thickness;

  // Calculate new absorber assembly parameters
  G4double absorberAssemblyThickness = fAbsorberThickness + 2 * fGoldThickness; // Length of absorber assembly (absorber + 2 gold foils)
  fAbsorberAssemblyDisplacement = fAbsorberAssemblyDistance + absorberAssemblyThickness / 2;

  // Calculate new gold foil displacement within absorber assembly
  G4double goldDisplacement = (fAbsorberThickness + fGoldThickness) / 2; // Displacement of gold foil from absorber assembly center

  // Update absorber and absorber assembly dimensions and positions
  fAbsorberTube->SetZHalfLength(fAbsorberThickness / 2);
  fAbsorberAssemblyTube->SetZHalfLength(absorberAssemblyThickness / 2);
  fAbsorberAssemblyPV->SetTranslation(G4ThreeVector(0, 0, fAbsorberAssemblyDisplacement));

  // Update gold foil positions within absorber assembly
  fGoldFrontPV->SetTranslation(G4ThreeVector(0, 0, -goldDisplacement));
  fGoldBackPV->SetTranslation(G4ThreeVector(0, 0, goldDisplacement));

  G4cout << "New absorber thickness: " << G4BestUnit(fAbsorberThickness, "Length") << G4endl;

  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization if it is active
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }

}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set absorber radius
//------------------------------------------------------------------------//
void DetectorConstruction::SetAbsorberRadius(G4double radius)
{
  // Check that geometry has been constructed
  if (!fAbsorberAssemblyPV) {
    G4cerr << "Absorber not yet constructed." << G4endl;
    return;
  }

  fAbsorberRadius = radius;

  fAbsorberTube->SetOuterRadius(fAbsorberRadius);
  fAbsorberAssemblyTube->SetOuterRadius(fAbsorberRadius);
  fGoldFrontTube->SetOuterRadius(fAbsorberRadius);
  fGoldBackTube->SetOuterRadius(fAbsorberRadius);

  G4cout << "New absorber radius: " << G4BestUnit(fAbsorberRadius, "Length") << G4endl;

  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization if it is active
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set absorber distance
//------------------------------------------------------------------------//
void DetectorConstruction::SetAbsorberDistance(G4double distance)
{
  // Check that geometry has been constructed
  if (!fAbsorberAssemblyPV) {
    G4cerr << "Absorber not yet constructed." << G4endl;
    return;
  }

  fAbsorberAssemblyDistance = distance;

  G4cout << "New absorber distance (to gold face): " << G4BestUnit(fAbsorberAssemblyDistance, "Length") << G4endl;

  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization if it is active
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set absorber spanning angle (phi) start
//------------------------------------------------------------------------//
void DetectorConstruction::SetSpanningStartAngle(G4double startAngle)
{
  // Check that geometry has been constructed
  if (!fAbsorberAssemblyPV) {
    G4cerr << "Absorber not yet constructed." << G4endl;
    return;
  }

  fStartAngle = startAngle;

  fAbsorberTube->SetStartPhiAngle(fStartAngle);
  fGoldFrontTube->SetStartPhiAngle(fStartAngle);
  fGoldBackTube->SetStartPhiAngle(fStartAngle);

  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization if it is active
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set absorber assembly spanning angle (phi) end
//------------------------------------------------------------------------//
void DetectorConstruction::SetSpanningEndAngle(G4double endAngle)
{
  // Check that geometry has been constructed
  if (!fAbsorberAssemblyPV) {
    G4cerr << "Absorber not yet constructed." << G4endl;
    return;
  }

  fEndAngle = endAngle;

  fAbsorberTube->SetDeltaPhiAngle(fEndAngle - fStartAngle);
  fGoldFrontTube->SetDeltaPhiAngle(fEndAngle - fStartAngle);
  fGoldBackTube->SetDeltaPhiAngle(fEndAngle - fStartAngle);

  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization if it is active
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Print detector construction parameters
//------------------------------------------------------------------------//
void DetectorConstruction::PrintParameters()
{
  G4cout << G4endl;
  G4cout << "============================================================" << G4endl;
  G4cout << "                   Geometry Configuration                   " << G4endl;
  G4cout << "============================================================" << G4endl;
  G4cout << "World size: " << G4BestUnit(kWorldSize, "Length") << G4endl;
  G4cout << "World material: " << fWorldMaterial->GetName() << G4endl;
  G4cout << "\nNeutrons generated at origin (0,0,0)" << G4endl;
  
  G4cout << "\nLi-6 Glass Detector (Cylindrical):" << G4endl;
  G4cout << "  Diameter: " << G4BestUnit(2*kDetectorRadius, "Length") << G4endl;
  G4cout << "  Thickness: " << G4BestUnit(kDetectorLength, "Length") << G4endl;
  G4cout << "  Detector face distance from origin: " << G4BestUnit(fDetectorDistance, "Length") << G4endl;
  G4cout << "  Material: " << fLi6GlassMaterial->GetName() << G4endl;
  G4cout << "  Density: " << fLi6GlassMaterial->GetDensity()/(g/cm3) << " g/cm3" << G4endl;

  G4cout << "\nAbsorber (Cylindrical):" << G4endl;
  G4cout << "  Material: " << fAbsorberMaterial->GetName() << G4endl;
  G4cout << "  Thickness: " << G4BestUnit(fAbsorberThickness, "Length") << G4endl;
  G4cout << "  Radius: " << G4BestUnit(fAbsorberRadius, "Length") << G4endl;
  G4cout << "  Density: " << fAbsorberMaterial->GetDensity()/(g/cm3) << " g/cm3" << G4endl;
  G4cout << "  Spanning angle (phi): " << fStartAngle / degree << " to " << fEndAngle / degree << " degrees" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......