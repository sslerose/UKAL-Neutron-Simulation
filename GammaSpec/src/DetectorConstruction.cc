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
  // Math for detector assembly placement
  //========================================================================//

  // Detector placement (distance from origin to detector face)
  fDetectorDisplacement = fDetectorDistance + kEncLength / 2;
  
  // Initialize materials and detector messenger
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

//------------------------------------------------------------------------//
// Define materials using NIST database and custom formulations
//------------------------------------------------------------------------//
void DetectorConstruction::DefineMaterials()
{
  // Obtain NIST material manager
  G4NistManager* nist = G4NistManager::Instance();
  
  // World material - Air
  fWorldMaterial = nist->FindOrBuildMaterial("G4_AIR");


  //========================================================================//
  // Get/Formulate elements and materials for detector assembly components
  //========================================================================//
  
  // Get pure elements
  fHPGEMaterial = nist->FindOrBuildMaterial("G4_Ge");     // Germanium
  fAlMaterial = nist->FindOrBuildMaterial("G4_Al");     // Aluminum

  // Get materials
  fMylarMaterial = nist->FindOrBuildMaterial("G4_MYLAR");   // Mylar


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
  // Detector geometry
  //========================================================================//

  fRotationMatrix[0] = new G4RotationMatrix();
  fRotationMatrix[0]->rotateY(0 * deg); // Front HPGe - no rotation

  fRotationMatrix[1] = new G4RotationMatrix();
  fRotationMatrix[1]->rotateY(180 * deg); // Back HPGe - rotated 180 degrees about Y axis

  for (G4int i = 0; i < 2; ++i) {
    G4LogicalVolume* detectorLV = BuildDetector(i);

    G4double sign = (i == 0) ? +1.0 : -1.0;

    fDetectorPV[i] = new G4PVPlacement(fRotationMatrix[i],
                                        G4ThreeVector(0, 0, sign * fDetectorDisplacement),
                                        detectorLV,
                                        (i == 0) ? "Detector_1" : "Detector_2",
                                        fWorldLV,
                                        false,
                                        i);
  }
  
  //========================================================================//
  // Absorber geometry (cylindrical)
  //========================================================================//

  // Absorber assembly volume

  G4double absorberAssemblyThickness = fAbsorberThickness + 2 * fGoldThickness; // Length of absorber assembly (absorber + 2 gold foils)

  // Absorber assembly placement

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
                                          G4ThreeVector(0, 0, 0), // position
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
// Build one full detector hierarchy (encasement mother and contents).
// Index (0 or 1) names the active crystal LV: HPGE_LV_1 / HPGE_LV_2.
//------------------------------------------------------------------------//
G4LogicalVolume* DetectorConstruction::BuildDetector(G4int index)
{
  //========================================================================//
  // Derived dimensions
  //========================================================================//

  // Germanium structure (active crystal + dead layer)
  G4double geOuterRadius = kCrystalRadius + kDeadOuter;   // incl. side dead layer
  G4double geLength      = kCrystalLength + kDeadOuter;   // incl. front dead layer

  // Cup: 2 mm radial gap between the germanium structure and the cup wall
  G4double cupInnerRadius = geOuterRadius + kGapCupGe;
  G4double cupOuterRadius = cupInnerRadius + kCupThickness;

  // Encasement: gap G maintained at front and around the sides
  G4double encInnerRadius = cupOuterRadius + kGapEncCup;
  G4double encOuterRadius = encInnerRadius + kEncThickness;


  //========================================================================//
  // Encasement
  //========================================================================//

  // Mother volume
  G4Tubs* encMotherTube = new G4Tubs("EncasementMother",
                                      0.0,
                                      encOuterRadius,
                                      kEncLength / 2,
                                      0.0, 360.0 * deg);

  G4LogicalVolume* encMotherLV = new G4LogicalVolume(encMotherTube,
                                                     fWorldMaterial,
                                                     "EncasementMother");

  // Front cap (full-width disk, flush with mother front)
  G4Tubs* encCapTube = new G4Tubs("EncasementCap",
                                  0.0,
                                  encInnerRadius,
                                  kEncThickness / 2,
                                  0.0, 360.0 * deg);

  G4LogicalVolume* encCapLV = new G4LogicalVolume(encCapTube,
                                                  fAlMaterial,
                                                  "EncasementCap");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, -kEncLength / 2 + kEncThickness / 2),
                    encCapLV,
                    "EncasementCap",
                    encMotherLV,
                    false,
                    index);

  // Side walls
  G4Tubs* encWallTube = new G4Tubs("EncasementWall",
                                    encInnerRadius,
                                    encOuterRadius,
                                    kEncLength / 2,
                                    0.0, 360.0 * deg);

  G4LogicalVolume* encWallLV = new G4LogicalVolume(encWallTube,
                                                    fAlMaterial,
                                                    "EncasementWall");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, 0),
                    encWallLV,
                    "EncasementWall",
                    encMotherLV,
                    false,
                    index);


  //========================================================================//
  // Cup
  //========================================================================//

  // Mother volume
  G4Tubs* cupMotherTube = new G4Tubs("CupMother",
                                     0.0,
                                     cupOuterRadius,
                                     kCupLength / 2,
                                     0.0, 360.0 * deg);

  G4LogicalVolume* cupMotherLV = new G4LogicalVolume(cupMotherTube,
                                                     fWorldMaterial,
                                                     "CupMother");

  G4double cupMotherZ = -kEncLength / 2 + kEncThickness + kGapEncCup + kCupLength / 2; // Add gap between encasement and cup

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, cupMotherZ),
                    cupMotherLV,
                    "CupMother",
                    encMotherLV,
                    false,
                    index);

  // Cup walls
  G4Tubs* cupTube = new G4Tubs("Cup",
                               cupInnerRadius,
                               cupOuterRadius,
                               kCupLength / 2,
                               0.0, 360.0 * deg);

  G4LogicalVolume* cupLV = new G4LogicalVolume(cupTube,
                                               fAlMaterial,
                                               "Cup");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, 0),
                    cupLV,
                    "Cup",
                    cupMotherLV,
                    false,
                    index);

  // Front cup cap aluminum layer
  G4Tubs* capAlTube = new G4Tubs("CupCapAl",
                                 0.0,
                                 cupInnerRadius,
                                 kCapAlThickness / 2,
                                 0.0, 360.0 * deg);

  G4LogicalVolume* capAlLV = new G4LogicalVolume(capAlTube,
                                                 fAlMaterial,
                                                 "CupCapAl");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, -kCupLength / 2 + kCapAlThickness / 2),
                    capAlLV,
                    "CupCapAl",
                    cupMotherLV,
                    false,
                    index);

  // Front cup cap Mylar layer
  G4Tubs* capMylarTube = new G4Tubs("CupCapMylar",
                                    0.0,
                                    cupInnerRadius,
                                    kCapMylarThickness / 2,
                                    0.0, 360.0 * deg);

  G4LogicalVolume* capMylarLV = new G4LogicalVolume(capMylarTube,
                                                    fMylarMaterial,
                                                    "CupCapMylar");

  G4double capMylarZ = -kCupLength / 2 + kCapAlThickness + kCapMylarThickness / 2; // Position Mylar layer behind Al layer

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, capMylarZ),
                    capMylarLV,
                    "CupCapMylar",
                    cupMotherLV,
                    false,
                    index);

  // Rear cup cap
  G4Tubs* backCapTube = new G4Tubs("CupBackCap",
                                   kHoleRadius,
                                   cupInnerRadius,
                                   kBackCapThickness / 2,
                                   0.0, 360.0 * deg);

  G4LogicalVolume* backCapLV = new G4LogicalVolume(backCapTube,
                                                   fAlMaterial,
                                                   "CupBackCap");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, kCupLength / 2 - kBackCapThickness / 2),
                    backCapLV,
                    "CupBackCap",
                    cupMotherLV,
                    false,
                    index);


  //========================================================================//
  // Germanium structure (active crystal + dead layer)
  //========================================================================//

  // Mother volume
  G4Tubs* geMotherTube = new G4Tubs("GeMother",
                                    0.0,
                                    geOuterRadius,
                                    geLength / 2,
                                    0.0, 360.0 * deg);

  G4LogicalVolume* geMotherLV = new G4LogicalVolume(geMotherTube,
                                                    fWorldMaterial,
                                                    "GeMother");

  G4double geMotherZ = -kCupLength / 2 + kCapAlThickness + kCapMylarThickness + geLength / 2; // Front face flush to the Mylar layer of cup

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, geMotherZ),
                    geMotherLV,
                    "GeMother",
                    cupMotherLV,
                    false,
                    index);

  // Outer dead layer face
  G4Tubs* deadFrontTube = new G4Tubs("DeadLayerFront",
                                     0.0,
                                     geOuterRadius,
                                     kDeadOuter / 2,
                                     0.0, 360.0 * deg);

  G4LogicalVolume* deadFrontLV = new G4LogicalVolume(deadFrontTube,
                                                     fHPGEMaterial,
                                                     "DeadLayerFront");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, -geLength / 2 + kDeadOuter / 2),
                    deadFrontLV,
                    "DeadLayerFront",
                    geMotherLV,
                    false,
                    index);

  // Outer dead layer side
  G4Tubs* deadSideTube = new G4Tubs("DeadLayerSide",
                                    kCrystalRadius,
                                    geOuterRadius,
                                    kCrystalLength / 2,
                                    0.0, 360.0 * deg);

  G4LogicalVolume* deadSideLV = new G4LogicalVolume(deadSideTube,
                                                    fHPGEMaterial,
                                                    "DeadLayerSide");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, kDeadOuter / 2),
                    deadSideLV,
                    "DeadLayerSide",
                    geMotherLV,
                    false,
                    index);

  // Active coaxial crystal
  G4Tubs* crystalOuterTube = new G4Tubs("ActiveCrystalOuter",
                                        0.0,
                                        kCrystalRadius,
                                        kCrystalLength / 2,
                                        0.0, 360.0 * deg);

  G4double boreClearance = 1.0 * mm;  // extra length for a clean subtraction

  G4Tubs* boreTube = new G4Tubs("CrystalBore",
                                0.0,
                                kHoleRadius,
                                (kHoleDepth + boreClearance) / 2,
                                0.0, 360.0 * deg);

  G4ThreeVector borePosition(0, 0, kCrystalLength / 2 - kHoleDepth / 2 + boreClearance / 2);  // Bore placement

  G4SubtractionSolid* crystalSolid = new G4SubtractionSolid("ActiveCrystal",
                                                            crystalOuterTube,
                                                            boreTube,
                                                            0,
                                                            borePosition);

  G4String activeName = (index == 0) ? "HPGE_LV_1" : "HPGE_LV_2"; // LV names for active Germanium crystals

  fActiveHPGELV[index] = new G4LogicalVolume(crystalSolid,
                                             fHPGEMaterial,
                                             activeName);

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, kDeadOuter / 2),
                    fActiveHPGELV[index],
                    activeName,
                    geMotherLV,
                    false,
                    index);

  // Inner dead layer cap
  G4Tubs* deadBoreBottomTube = new G4Tubs("DeadLayerBoreBottom",
                                          0.0,
                                          kHoleRadius,
                                          kDeadInner / 2,
                                          0.0, 360.0 * deg);

  G4LogicalVolume* deadBoreBottomLV = new G4LogicalVolume(deadBoreBottomTube,
                                                          fHPGEMaterial,
                                                          "DeadLayerBoreBottom");

  G4double boreBottomZ  = geLength / 2 - kHoleDepth;  // Bore placement

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, boreBottomZ + kDeadInner / 2),
                    deadBoreBottomLV,
                    "DeadLayerBoreBottom",
                    geMotherLV,
                    false,
                    index);

  // Inner dead layer walls
  G4double boreWallLength = kHoleDepth - kDeadInner;

  G4Tubs* deadBoreWallTube = new G4Tubs("DeadLayerBoreWall",
                                        kHoleRadius - kDeadInner,
                                        kHoleRadius,
                                        boreWallLength / 2,
                                        0.0, 360.0 * deg);

  G4LogicalVolume* deadBoreWallLV = new G4LogicalVolume(deadBoreWallTube,
                                                        fHPGEMaterial,
                                                        "DeadLayerBoreWall");

  new G4PVPlacement(0,
                    G4ThreeVector(0, 0, geLength / 2 - boreWallLength / 2),
                    deadBoreWallLV,
                    "DeadLayerBoreWall",
                    geMotherLV,
                    false,
                    index);

  return encMotherLV;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set HPGe detector distance from origin
//------------------------------------------------------------------------//
void DetectorConstruction::SetDetectorDistance(G4double value)
{
  // Check that geometry has been constructed
  if (!fDetectorPV[0] || !fDetectorPV[1]) {
    G4cerr << "Detector not yet constructed." << G4endl;
    return;
  }

  // Calculate new assembly coordinates accounting for assembly angle
  fDetectorDistance = value;  // Distance from origin to detector face
  fDetectorDisplacement = fDetectorDistance + kEncLength / 2;  // Distance from origin to detector center

  // Update assembly position
  fDetectorPV[0]->SetTranslation(G4ThreeVector(0, 0, fDetectorDisplacement));
  fDetectorPV[1]->SetTranslation(G4ThreeVector(0, 0, -fDetectorDisplacement));

  G4cout << "New detector distance to source: " << G4BestUnit(fDetectorDistance, "Length") << G4endl;

  // Notify run manager of geometry modification
  G4RunManager::GetRunManager()->GeometryHasBeenModified();

  // Update visualization (if it is active)
  G4VVisManager* visManager = G4VVisManager::GetConcreteInstance();
  if (visManager) {
    // Visualization is active - trigger an update
    G4UImanager::GetUIpointer()->ApplyCommand("/vis/scene/notifyHandlers");
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set world material
//------------------------------------------------------------------------//
void DetectorConstruction::SetWorldMaterial(G4String value)
{
  // Check that geometry has been constructed
  if (!fWorldPV) {
    G4cerr << "World not yet constructed." << G4endl;
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

  // Calculate new gold foil displacement within absorber assembly
  G4double goldDisplacement = (fAbsorberThickness + fGoldThickness) / 2; // Displacement of gold foil from absorber assembly center

  // Update absorber and absorber assembly dimensions and positions
  fAbsorberTube->SetZHalfLength(fAbsorberThickness / 2);
  fAbsorberAssemblyTube->SetZHalfLength(absorberAssemblyThickness / 2);
  fAbsorberAssemblyPV->SetTranslation(G4ThreeVector(0, 0, 0));

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
  
  // G4cout << "\nHPGe Detector (Coaxial):" << G4endl;
  // G4cout << "  Diameter: " << G4BestUnit(2*kHPGEOuterRadius, "Length") << G4endl;
  // G4cout << "  Length: " << G4BestUnit(kHPGELength, "Length") << G4endl;
  // G4cout << "  Detector face distance from origin: " << G4BestUnit(fHPGEDistance, "Length") << G4endl;
  // G4cout << "  Material: " << fHPGEMaterial->GetName() << G4endl;
  // G4cout << "  Density: " << fHPGEMaterial->GetDensity()/(g/cm3) << " g/cm3" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......