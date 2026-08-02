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
  fHPGEDisplacement = fHPGEDistance + kCryoHalfLength;
  
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
  
  // World materials
  fWorldMaterial = nist->FindOrBuildMaterial("G4_AIR");


  //========================================================================//
  // Define detector materials
  //========================================================================//
  
  // Get pure elements
  fHPGEMaterial = nist->FindOrBuildMaterial("G4_Ge");         // Germanium
  fAluminumMaterial = nist->FindOrBuildMaterial("G4_Al");     // Aluminum
  fVacuumMaterial = nist->FindOrBuildMaterial("G4_Galactic"); // Vacuum


  //========================================================================//
  // Define absorber materials
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
  // Detector Mother Volumes
  //========================================================================//

  G4Tubs* motherSolid = new G4Tubs("DetectorMother",
                                     0.,
                                     kMotherROut,
                                     kMotherHalfLength,
                                     0., 360.*deg);

  fHPGEMotherLV[0] = new G4LogicalVolume(motherSolid,
                                         fVacuumMaterial,
                                         "DetectorMother_1");

  fHPGEMotherLV[1] = new G4LogicalVolume(motherSolid,
                                         fVacuumMaterial,
                                         "DetectorMother_2");

  fRotationMatrix[0] = new G4RotationMatrix();
  fRotationMatrix[0]->rotateY(0 * deg); // Front HPGe - no rotation

  fRotationMatrix[1] = new G4RotationMatrix();
  fRotationMatrix[1]->rotateY(180 * deg); // Back HPGe - rotated 180 degrees about Y axis

  fHPGEMotherPV[0] = new G4PVPlacement(fRotationMatrix[0],
                                       G4ThreeVector(0, 0, fHPGEDisplacement),
                                       fHPGEMotherLV[0],
                                       "DetectorMother_1",
                                       fWorldLV,
                                       false,
                                       0);

  fHPGEMotherPV[1] = new G4PVPlacement(fRotationMatrix[1],
                                       G4ThreeVector(0, 0, -fHPGEDisplacement),
                                       fHPGEMotherLV[1],
                                       "DetectorMother_2",
                                       fWorldLV,
                                       false,
                                       0);



  //========================================================================//
  // Detector geometry
  // Particles generated at origin
  //========================================================================//

  // Solids
  // G4Tubs* outerTube = new G4Tubs("HPGE_Outer", 
  //                                 0, 
  //                                 kHPGEOuterRadius, 
  //                                 kHPGELength / 2, 
  //                                 0, 
  //                                 360 * deg);

  // G4Tubs* boreTube = new G4Tubs("HPGE_Bore",
  //                                0,
  //                                kHPGEInnerRadius,
  //                                kHPGEBoreDepth / 2 + kHPGEBoreDepth * 0.1, // Add extra length to ensure complete subtraction
  //                                0,
  //                                360 * deg);

  // G4ThreeVector borePosition(0, 0, (kHPGELength - kHPGEBoreDepth) / 2 + kHPGEBoreDepth * 0.1);

  // G4SubtractionSolid* hpgeSolid = new G4SubtractionSolid("HPGE_Solid", 
  //                                                         outerTube, 
  //                                                         boreTube, 
  //                                                         0, 
  //                                                         borePosition);

  // // Logical volumes
  // fHPGELV[0] = new G4LogicalVolume(hpgeSolid, 
  //                                  fHPGEMaterial, 
  //                                  "HPGE_LV_1");

  // fHPGELV[1] = new G4LogicalVolume(hpgeSolid, 
  //                                  fHPGEMaterial, 
  //                                  "HPGE_LV_2");

  // // Physical volumes
  // fRotationMatrix[0] = new G4RotationMatrix();
  // fRotationMatrix[0]->rotateY(0 * deg); // Front HPGe - no rotation

  // fRotationMatrix[1] = new G4RotationMatrix();
  // fRotationMatrix[1]->rotateY(180 * deg); // Back HPGe - rotated 180 degrees about Y axis

  // fHPGEPV[0] = new G4PVPlacement(fRotationMatrix[0],
  //                               G4ThreeVector(0, 0, fHPGEDisplacement),
  //                               fHPGELV[0],
  //                               "HPGE_PV_1",
  //                               fWorldLV,
  //                               false,
  //                               0);

  // fHPGEPV[1] = new G4PVPlacement(fRotationMatrix[1],
  //                               G4ThreeVector(0, 0, -fHPGEDisplacement),
  //                               fHPGELV[1],
  //                               "HPGE_PV_2",
  //                               fWorldLV,
  //                               false,
  //                               0);

  
  //========================================================================//
  // Absorber geometry (cylindrical)
  //========================================================================//

  // Absorber assembly placement
  G4double absorberAssemblyThickness = fAbsorberThickness + 2 * fGoldThickness; // Length of absorber assembly (absorber + 2 gold foils)

  // Absorber assembly volume

  fAbsorberAssemblyTube = new G4Tubs("AbsorberAssembly",
                                            0.0,                            // inner radius (solid cylinder)
                                            fAbsorberRadius,                // outer radius
                                            absorberAssemblyThickness / 2,  // half length
                                            fStartAngle,                    // initial spanning angle
                                            fEndAngle);                     // final spanning angle

  fAbsorberAssemblyLV = new G4LogicalVolume(fAbsorberAssemblyTube,
                                            fWorldMaterial,
                                            "AbsorberAssembly");

  fAbsorberAssemblyPV = new G4PVPlacement(0,                        // no rotation
                                          G4ThreeVector(0, 0, 0),   // position
                                          fAbsorberAssemblyLV,      // logical volume
                                          "AbsorberAssembly",       // name
                                          fWorldLV,                 // mother volume
                                          false,                    // no boolean operation
                                          0);                       // copy number


  // Gold foil (front)

  G4double goldDisplacement = (fAbsorberThickness + fGoldThickness) / 2; // Displacement of gold foil from absorber assembly center

  fGoldFrontTube = new G4Tubs("GoldFrontFoil",
                              0.0,                // inner radius (solid cylinder)
                              fAbsorberRadius,    // outer radius
                              fGoldThickness / 2, // half length
                              fStartAngle,        // initial spanning angle
                              fEndAngle);         // final spanning angle

  fGoldLV = new G4LogicalVolume(fGoldFrontTube,
                                fGoldMaterial,
                                "GoldFrontFoil");

  fGoldFrontPV = new G4PVPlacement(0,                                        // no rotation
                                    G4ThreeVector(0, 0, -goldDisplacement),  // position
                                    fGoldLV,                            // logical volume
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

  // fGoldBackLV = new G4LogicalVolume(fGoldBackTube,
  //                                   fGoldMaterial,
  //                                   "GoldBackFoil");

  fGoldBackPV = new G4PVPlacement(0,                                      // no rotation
                                  G4ThreeVector(0, 0, goldDisplacement),  // position
                                  fGoldLV,                            // logical volume
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

  new G4PVPlacement(0,                      // no rotation
                    G4ThreeVector(0, 0, 0), // position
                    fAbsorberLV,            // logical volume
                    "Absorber",             // name
                    fAbsorberAssemblyLV,    // mother volume
                    false,                  // no boolean operation
                    0);                     // copy number


  //========================================================================//
  // Build nested detector geometry within mother volumes
  //========================================================================//

  BuildDetectorStack(fHPGEMotherLV[0], 0);
  BuildDetectorStack(fHPGEMotherLV[1], 1);


  // Return the root volume
  return fWorldPV;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::BuildDetectorStack(G4LogicalVolume* motherLV, G4int detectorID)
{
    const G4bool firstBuild = (detectorID == 0);

    //========================================================================//
    // Cryostat — Al shell
    //   rIn  = kCryoRIn,  rOut = kCryoROut
    //   Parent: motherLV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("Cryostat",
                                  kCryoRIn,
                                  kCryoROut,
                                  kCryoHalfLength,
                                  0., 360.*deg);

      fCryostatLV = new G4LogicalVolume(solid,
                                        fAluminumMaterial,
                                        "Cryostat");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fCryostatLV,
                      "Cryostat",
                      motherLV,
                      false,
                      detectorID);


    //========================================================================//
    // Vacuum2 — gap between cryostat and detector cup
    //   rIn  = kVac2RIn,  rOut = kVac2ROut
    //   Parent: fCryostatLV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("Vacuum2",
                                  kVac2RIn,
                                  kVac2ROut,
                                  kVac2HalfLength,
                                  0., 360.*deg);
                                  
      fVacuum2LV = new G4LogicalVolume(solid,
                                        fVacuumMaterial,
                                        "Vacuum2");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fVacuum2LV,
                      "Vacuum2",
                      fCryostatLV,
                      false,
                      detectorID);


    //========================================================================//
    // DetectorCup — Al structural cup
    //   rIn  = kDetCupRIn,  rOut = kDetCupROut
    //   Parent: fVacuum2LV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("DetectorCup",
                                  kDetCupRIn,
                                  kDetCupROut,
                                  kDetCupHalfLength,
                                  0., 360.*deg);
                                  
      fDetectorCupLV = new G4LogicalVolume(solid,
                                            fAluminumMaterial,
                                            "DetectorCup");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fDetectorCupLV,
                      "DetectorCup",
                      fVacuum2LV,
                      false,
                      detectorID);


    //========================================================================//
    // Vacuum1 — gap between detector cup and Ge crystal wall
    //   rIn  = kVac1RIn,  rOut = kVac1ROut
    //   Parent: fDetectorCupLV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("Vacuum1",
                                  kVac1RIn,
                                  kVac1ROut,
                                  kVac1HalfLength,
                                  0., 360.*deg);
                                  
      fVacuum1LV = new G4LogicalVolume(solid,
                                        fVacuumMaterial,
                                        "Vacuum1");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fVacuum1LV,
                      "Vacuum1",
                      fDetectorCupLV,
                      false,
                      detectorID);


    //========================================================================//
    // OuterDeadLayer — Li-diffused Ge contact
    //   rIn  = kOuterDeadRIn,  rOut = kOuterDeadROut
    //   Parent: fVacuum1LV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("OuterDeadLayer",
                                  kOuterDeadRIn,
                                  kOuterDeadROut,
                                  kOuterDeadHalfLength,
                                  0., 360.*deg);
                                  
      fOuterDeadLayerLV = new G4LogicalVolume(solid,
                                              fHPGEMaterial,
                                              "OuterDeadLayer");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fOuterDeadLayerLV,
                      "OuterDeadLayer",
                      fVacuum1LV,
                      false,
                      detectorID);


    //========================================================================//
    // ActiveCrystal — bulk active Ge
    //   rIn  = kActiveRIn,  rOut = kActiveROut
    //   Parent: fOuterDeadLayerLV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("ActiveCrystal",
                                  kActiveRIn,
                                  kActiveROut,
                                  kActiveHalfLength,
                                  0., 360.*deg);
                                  
      fActiveCrystalLV = new G4LogicalVolume(solid,
                                              fHPGEMaterial,
                                              "ActiveCrystal");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fActiveCrystalLV,
                      "ActiveCrystal",
                      fOuterDeadLayerLV,
                      false,
                      detectorID);


    //========================================================================//
    // InnerDeadLayer — B-implanted Ge contact
    //   rIn  = kInnerDeadRIn,  rOut = kInnerDeadROut
    //   Parent: fActiveCrystalLV
    //========================================================================//

    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("InnerDeadLayer",
                                  kInnerDeadRIn,
                                  kInnerDeadROut,
                                  kInnerDeadHalfLength,
                                  0., 360.*deg);
                                  
      fInnerDeadLayerLV = new G4LogicalVolume(solid,
                                              fHPGEMaterial,
                                              "InnerDeadLayer");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fInnerDeadLayerLV,
                      "InnerDeadLayer",
                      fActiveCrystalLV,
                      false,
                      detectorID);


    //========================================================================//
    // CentralContact — vacuum bore
    //   rIn  = 0,  rOut = kCentralROut
    //   Parent: fInnerDeadLayerLV
    //========================================================================//
    
    if (firstBuild) {
      G4Tubs* solid = new G4Tubs("CentralContact",
                                  0.,
                                  kCentralROut,
                                  kCentralHalfLength,
                                  0., 360.*deg);
                                  
      fCentralContactLV = new G4LogicalVolume(solid,
                                              fVacuumMaterial,
                                              "CentralContact");
    }

    new G4PVPlacement(0,
                      G4ThreeVector(),
                      fCentralContactLV,
                      "CentralContact",
                      fInnerDeadLayerLV,
                      false,
                      detectorID);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Set HPGe detector distance from origin
//------------------------------------------------------------------------//
void DetectorConstruction::SetDetectorDistance(G4double value)
{
  // Check that geometry has been constructed
  if (!fHPGEMotherPV[0] || !fHPGEMotherPV[1]) {
    G4cerr << "Detector not yet constructed." << G4endl;
    return;
  }

  // Calculate new assembly coordinates accounting for assembly angle
  fHPGEDistance = value;  // Distance from origin to detector face
  fHPGEDisplacement = fHPGEDistance + kCryoHalfLength;  // Distance from origin to detector center

  // Update assembly position
  fHPGEMotherPV[0]->SetTranslation(G4ThreeVector(0, 0, fHPGEDisplacement));
  fHPGEMotherPV[1]->SetTranslation(G4ThreeVector(0, 0, -fHPGEDisplacement));

  G4cout << "New HPGe detector distance to source: " << G4BestUnit(fHPGEDistance, "Length") << G4endl;

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
  
  G4cout << "\nHPGe Detector (Coaxial):" << G4endl;
  // G4cout << "  Diameter: " << G4BestUnit(2*kHPGEOuterRadius, "Length") << G4endl;
  // G4cout << "  Length: " << G4BestUnit(kHPGELength, "Length") << G4endl;
  G4cout << "  Detector face distance from origin: " << G4BestUnit(fHPGEDistance, "Length") << G4endl;
  G4cout << "  Material: " << fHPGEMaterial->GetName() << G4endl;
  G4cout << "  Density: " << fHPGEMaterial->GetDensity()/(g/cm3) << " g/cm3" << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......