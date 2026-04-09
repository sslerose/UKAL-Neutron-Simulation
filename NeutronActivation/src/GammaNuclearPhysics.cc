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

//
/// \file GammaNuclearPhysics.cc
/// \brief Implementation of the GammaNuclearPhysics class
//

#include "GammaNuclearPhysics.hh"

#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"

// Processes

#include "G4CascadeInterface.hh"
#include "G4HadronInelasticProcess.hh"
#include "G4LowEGammaNuclearModel.hh"
#include "G4PhotoNuclearCrossSection.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

GammaNuclearPhysics::GammaNuclearPhysics(const G4String& name) : G4VPhysicsConstructor(name) {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void GammaNuclearPhysics::ConstructProcess()
{
  G4HadronInelasticProcess* photoNuclearProcess =
    new G4HadronInelasticProcess("photonNuclear", G4Gamma::Definition());
  photoNuclearProcess->AddDataSet(new G4PhotoNuclearCrossSection);

  // To not register a model, set Emax=0; eg. lowEmax = 0.
  const G4double lowEmax = 200 * MeV, cascadeEmax = 10 * GeV;

  if (lowEmax > 0.) {  // Low energy model
    G4LowEGammaNuclearModel* theLowEmodel = new G4LowEGammaNuclearModel();
    theLowEmodel->SetMaxEnergy(lowEmax);
    photoNuclearProcess->RegisterMe(theLowEmodel);
  }

  if (cascadeEmax > 0.) {  // Cascade model
    G4CascadeInterface* theCascadeModel = new G4CascadeInterface();
    G4double Emin2 = std::max(lowEmax - 1 * MeV, 0.);
    theCascadeModel->SetMinEnergy(Emin2);
    theCascadeModel->SetMaxEnergy(cascadeEmax);
    photoNuclearProcess->RegisterMe(theCascadeModel);
  }

  G4ProcessManager* pManager = G4Gamma::Gamma()->GetProcessManager();
  pManager->AddDiscreteProcess(photoNuclearProcess);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
