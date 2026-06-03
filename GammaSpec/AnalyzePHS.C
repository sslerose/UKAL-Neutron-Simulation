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
// ********************************************************************
//

//
/// \file AnalyzePHS.C
/// \brief ROOT macro for generating a pulse-height spectrum from GammaSpec output.
///
/// Usage (in ROOT):
///   .L AnalyzePHS.C
///   analyzePHS("path/to/GammaSpec.root", window_us, nBins, eMax)
///
/// Required argument:
///   filePath   - path to the .root data file
///
/// Optional arguments:
///   window_us  - charge-collection window length in microseconds (default = 1.0)
///   nBins      - number of histogram bins (default = 3000)
///   eMax       - histogram upper edge in MeV (default = 3.0)
///
/// Outputs:
///   <stem>_PHS_<window_us>us.png  - pulse-height spectrum plot
///   <stem>_PHS_<window_us>us.csv  - bin-center (MeV) and counts
///
/// Algorithm:
///   Each row in EnergyDeposition is assigned to a time window
///   idx = floor(Time / window_us).  Within each window, the
///   weighted energy deposits are summed: pulse[idx] += EnergyDep * Weight.
///   One histogram entry is filled per occupied window.  Empty windows
///   are skipped entirely to avoid a spurious zero-energy peak.
//

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TString.h"
#include "TMath.h"
#include "TDatabasePDG.h"
#include "TParticlePDG.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// ============================================================
// Helper: apply consistent axis styling to a histogram
// ============================================================
void styleAxis(TH1D* h, const char* xTitle, const char* yTitle)
{
    h->GetXaxis()->SetTitle(xTitle);
    h->GetYaxis()->SetTitle(yTitle);
    h->GetXaxis()->SetTitleOffset(1.3);
    h->GetYaxis()->SetTitleOffset(1.5);
    h->GetXaxis()->SetLabelSize(0.04);
    h->GetYaxis()->SetLabelSize(0.04);
    h->GetXaxis()->SetTitleSize(0.04);
    h->GetYaxis()->SetTitleSize(0.04);
    h->GetXaxis()->CenterTitle(true);
    h->GetYaxis()->CenterTitle(true);
}

// ============================================================
// Main analysis function
// ============================================================
void analyzePHS(const char* filePath,
                double window_us = 1.0,
                int    nBins     = 3000,
                double eMax      = 3.0)
{
    // ---- derive output filename stem from input path ----
    TString inputPath(filePath);
    TString stem = inputPath;
    // strip leading directory
    int lastSlash = stem.Last('/');
    if (lastSlash >= 0) stem = stem(lastSlash + 1, stem.Length() - lastSlash - 1);
    // strip .root extension
    if (stem.EndsWith(".root")) stem.Remove(stem.Length() - 5, 5);

    std::ostringstream tagStream;
    tagStream << std::fixed << std::setprecision(2) << window_us;
    TString tag = TString(stem) + "_PHS_" + tagStream.str().c_str() + "us";

    // ---- open file and tree ----
    TFile* f = TFile::Open(filePath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error: cannot open \"" << filePath << "\"\n";
        return;
    }

    TTree* tree = dynamic_cast<TTree*>(f->Get("EnergyDeposition"));
    if (!tree) {
        std::cerr << "Error: \"EnergyDeposition\" tree not found in \"" << filePath << "\"\n";
        f->Close();
        return;
    }

    // ---- bind branches ----
    Double_t energyDep = 0.0;
    Double_t weight    = 0.0;
    Double_t time      = 0.0;

    tree->SetBranchAddress("EnergyDep", &energyDep);
    tree->SetBranchAddress("Weight",    &weight);
    tree->SetBranchAddress("Time",      &time);

    // ---- single-pass accumulation into time windows ----
    std::map<long long, double> windowSums;

    Long64_t nEntries = tree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        long long idx = static_cast<long long>(std::floor(time / window_us));
        windowSums[idx] += energyDep * weight;
    }

    long long nWindows = static_cast<long long>(windowSums.size());

    // ---- determine energy range of occupied windows for summary ----
    double minPulse =  1e30;
    double maxPulse = -1e30;
    for (auto& kv : windowSums) {
        if (kv.second < minPulse) minPulse = kv.second;
        if (kv.second > maxPulse) maxPulse = kv.second;
    }

    // ---- print summary ----
    std::cout << "\n=== Pulse-Height Spectrum Analysis ===\n"
              << "  File         : " << filePath  << "\n"
              << "  Window       : " << window_us << " us\n"
              << "  Rows read    : " << nEntries  << "\n"
              << "  Occupied windows: " << nWindows << "\n"
              << "  Pulse range  : [" << minPulse << ", " << maxPulse << "] MeV\n"
              << "=======================================\n\n";

    // ---- fill histogram ----
    gStyle->SetOptStat(0);

    TH1D* hPHS = new TH1D("hPHS", "Pulse-Height Spectrum",
                           nBins, 0.0, eMax);

    for (auto& kv : windowSums) {
        hPHS->Fill(kv.second);
    }

    styleAxis(hPHS, "Pulse Height (MeV)", "Counts");

    // ---- save PNG ----
    TCanvas* c = new TCanvas("cPHS", "Pulse-Height Spectrum", 800, 600);
    c->SetLeftMargin(0.12);
    c->SetBottomMargin(0.12);
    hPHS->Draw("HIST");
    TString pngName = tag + ".png";
    c->SaveAs(pngName.Data());
    std::cout << "Saved plot : " << pngName.Data() << "\n";

    // ---- save CSV ----
    TString csvName = tag + ".csv";
    std::ofstream csv(csvName.Data());
    csv << "BinCenter_MeV,Counts\n";
    for (int b = 1; b <= hPHS->GetNbinsX(); ++b) {
        csv << std::fixed << std::setprecision(6)
            << hPHS->GetBinCenter(b) << ","
            << hPHS->GetBinContent(b) << "\n";
    }
    csv.close();
    std::cout << "Saved data : " << csvName.Data() << "\n";

    // ---- EmittedParticles CSV + gamma histograms ----
    TTree* emTree = dynamic_cast<TTree*>(f->Get("EmittedParticles"));
    if (!emTree) {
        std::cerr << "Warning: \"EmittedParticles\" tree not found in \"" << filePath << "\"\n";
    } else {
        Int_t    emPID    = 0;
        Double_t emEnergy = 0.0;
        Double_t emWeight = 0.0;
        Double_t emTime   = 0.0;

        emTree->SetBranchAddress("PID",    &emPID);
        emTree->SetBranchAddress("Energy", &emEnergy);
        emTree->SetBranchAddress("Weight", &emWeight);
        emTree->SetBranchAddress("Time",   &emTime);

        TDatabasePDG* pdgDB = TDatabasePDG::Instance();

        TString emCsvName = TString(stem) + "_EmittedParticles.csv";
        std::ofstream emCsv(emCsvName.Data());
        emCsv << "PID,Name,Energy_MeV,Weight,Time_us\n";

        Long64_t emEntries  = emTree->GetEntries();
        Long64_t emCsvLimit = (emEntries + 9) / 10;

        TH1D* hGammaE   = new TH1D("hGammaE",   "Emitted Gamma Energy Spectrum",      nBins, 0.0, eMax);
        TH1D* hGammaPHS = new TH1D("hGammaPHS",  "Emitted Gamma Pulse-Height Spectrum", nBins, 0.0, eMax);
        std::map<long long, double> gammaWindowSums;

        for (Long64_t i = 0; i < emEntries; ++i) {
            emTree->GetEntry(i);
            if (i < emCsvLimit) {
                TParticlePDG* pdgParticle = pdgDB->GetParticle(emPID);
                const char* emName = pdgParticle ? pdgParticle->GetName() : "unknown";
                emCsv << emPID << ","
                      << emName << ","
                      << std::fixed << std::setprecision(6)
                      << emEnergy << ","
                      << emWeight << ","
                      << emTime   << "\n";
            }
            if (emPID == 22) {
                hGammaE->Fill(emEnergy);
                long long idx = static_cast<long long>(std::floor(emTime / window_us));
                gammaWindowSums[idx] += emEnergy * emWeight;
            }
        }
        emCsv.close();
        std::cout << "Saved data : " << emCsvName.Data()
                  << "  (first " << emCsvLimit << " of " << emEntries << " rows)\n";

        for (auto& kv : gammaWindowSums) hGammaPHS->Fill(kv.second);

        TString gammaETag  = TString(stem) + "_GammaEnergy";
        TString gammaPHSTag = TString(stem) + "_GammaPHS_" + tagStream.str().c_str() + "us";

        styleAxis(hGammaE,   "Energy (MeV)",      "Counts");
        styleAxis(hGammaPHS, "Pulse Height (MeV)", "Counts");

        TCanvas* cGammaE = new TCanvas("cGammaE", "Emitted Gamma Energy", 800, 600);
        cGammaE->SetLeftMargin(0.12);
        cGammaE->SetBottomMargin(0.12);
        hGammaE->Draw("HIST");
        TString gammaEPng = gammaETag + ".png";
        cGammaE->SaveAs(gammaEPng.Data());
        std::cout << "Saved plot : " << gammaEPng.Data() << "\n";

        TString gammaECsv = gammaETag + ".csv";
        std::ofstream gammaECsvFile(gammaECsv.Data());
        gammaECsvFile << "BinCenter_MeV,Counts\n";
        for (int b = 1; b <= hGammaE->GetNbinsX(); ++b) {
            gammaECsvFile << std::fixed << std::setprecision(6)
                          << hGammaE->GetBinCenter(b) << ","
                          << hGammaE->GetBinContent(b) << "\n";
        }
        gammaECsvFile.close();
        std::cout << "Saved data : " << gammaECsv.Data() << "\n";

        TCanvas* cGammaPHS = new TCanvas("cGammaPHS", "Emitted Gamma PHS", 800, 600);
        cGammaPHS->SetLeftMargin(0.12);
        cGammaPHS->SetBottomMargin(0.12);
        hGammaPHS->Draw("HIST");
        TString gammaPHSPng = gammaPHSTag + ".png";
        cGammaPHS->SaveAs(gammaPHSPng.Data());
        std::cout << "Saved plot : " << gammaPHSPng.Data() << "\n";

        delete cGammaE;
        delete cGammaPHS;
        delete hGammaE;
        delete hGammaPHS;
    }

    // ---- DecayProducts CSV ----
    TTree* dpTree = dynamic_cast<TTree*>(f->Get("DecayProducts"));
    if (!dpTree) {
        std::cerr << "Warning: \"DecayProducts\" tree not found in \"" << filePath << "\"\n";
    } else {
        Int_t    dpPID       = 0;
        Int_t    dpZ         = 0;
        Int_t    dpA         = 0;
        Double_t dpEnergy    = 0.0;
        Double_t dpWeight    = 0.0;
        Double_t dpTimeBirth = 0.0;
        Double_t dpTimeDeath = 0.0;

        dpTree->SetBranchAddress("PID",       &dpPID);
        dpTree->SetBranchAddress("Z",         &dpZ);
        dpTree->SetBranchAddress("A",         &dpA);
        dpTree->SetBranchAddress("Energy",    &dpEnergy);
        dpTree->SetBranchAddress("Weight",    &dpWeight);
        dpTree->SetBranchAddress("TimeBirth", &dpTimeBirth);
        dpTree->SetBranchAddress("TimeDeath", &dpTimeDeath);

        TString dpCsvName = TString(stem) + "_DecayProducts.csv";
        std::ofstream dpCsv(dpCsvName.Data());
        dpCsv << "PID,Z,A,Energy_MeV,Weight,TimeBirth_us,TimeDeath_us\n";

        Long64_t dpEntries  = dpTree->GetEntries();
        Long64_t dpCsvLimit = (dpEntries + 9) / 10;
        for (Long64_t i = 0; i < dpCsvLimit; ++i) {
            dpTree->GetEntry(i);
            dpCsv << dpPID << ","
                  << dpZ   << ","
                  << dpA   << ","
                  << std::fixed << std::setprecision(6)
                  << dpEnergy    << ","
                  << dpWeight    << ","
                  << dpTimeBirth << ","
                  << dpTimeDeath << "\n";
        }
        dpCsv.close();
        std::cout << "Saved data : " << dpCsvName.Data()
                  << "  (first " << dpCsvLimit << " of " << dpEntries << " rows)\n";
    }

    // ---- cleanup ----
    delete c;
    delete hPHS;
    f->Close();
}
