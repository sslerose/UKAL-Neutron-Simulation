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

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// ============================================================
// Run metadata for multi-file total PHS analysis
// ============================================================
struct RunInfo {
    TString file;  // ROOT file stem (no .root extension)
    double  P;     // true parent population from activation sim
    double  N;     // simulated parent population used in GammaSpec run
};

// Parse a whitespace-delimited run info file into a vector of RunInfo.
// Lines beginning with '#' are treated as comments and skipped.
std::vector<RunInfo> parseRunInfo(const char* infoPath)
{
    std::vector<RunInfo> runs;
    std::ifstream fin(infoPath);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open run info file \"" << infoPath << "\"\n";
        return runs;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string fname;
        double P, N;
        if (!(ss >> fname >> P >> N)) continue;
        RunInfo r;
        r.file = TString(fname);
        r.P    = P;
        r.N    = N;
        runs.push_back(r);
    }
    return runs;
}

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
    std::map<long long, double> windowEnergy;
    std::map<long long, double> windowWeighted;
    std::map<long long, double> windowWeightSum;

    Long64_t nEntries = tree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        long long idx = static_cast<long long>(std::floor(time / window_us));
        windowEnergy[idx]    += energyDep;
        windowWeighted[idx]  += energyDep * weight;
        windowWeightSum[idx] += weight;
    }

    long long nWindows = static_cast<long long>(windowEnergy.size());

    // ---- determine energy range of occupied windows for summary ----
    double minPulse =  1e30;
    double maxPulse = -1e30;
    for (auto& kv : windowEnergy) {
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

    // Pulse magnitude is the unweighted energy sum; the histogram fill weight
    // is the summed biasing weight of the deposits in the window
    for (auto& kv : windowEnergy) {
        double E    = kv.second;
        double Wsum = windowWeightSum[kv.first];
        hPHS->Fill(E, Wsum);
        // --- previous implementation: energy-weighted average of the weights ---
        // double Wavg = windowWeighted[kv.first] / E;
        // hPHS->Fill(E, Wavg);
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
        emCsv << "PID,Name,Energy_MeV,Weight,Time_s\n";

        Long64_t emEntries  = emTree->GetEntries();
        // Dump the first 10% of rows, capped at 10000
        Long64_t emCsvLimit = std::min<Long64_t>((emEntries + 9) / 10, 10000);

        TH1D* hGammaE = new TH1D("hGammaE", "Emitted Gamma Energy Spectrum", nBins, 0.0, eMax);

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
                      << emTime * 1e-6 << "\n";
            }
            if (emPID == 22) {
                hGammaE->Fill(emEnergy);
            }
        }
        emCsv.close();
        std::cout << "Saved data : " << emCsvName.Data()
                  << "  (first " << emCsvLimit << " of " << emEntries << " rows)\n";

        TString gammaETag = TString(stem) + "_GammaEnergy";

        styleAxis(hGammaE, "Energy (MeV)", "Counts");

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

        delete cGammaE;
        delete hGammaE;
    }

    // ---- DecayProducts CSV ----
    TTree* dpTree = dynamic_cast<TTree*>(f->Get("DecayProducts"));
    if (!dpTree) {
        std::cerr << "Warning: \"DecayProducts\" tree not found in \"" << filePath << "\"\n";
    } else {
        Int_t    dpPID        = 0;
        Int_t    dpZ          = 0;
        Int_t    dpA          = 0;
        Char_t   dpCreator[256] = {0};
        Double_t dpExcitation = 0.0;
        Double_t dpWeight     = 0.0;
        Int_t    dpIsStable   = 0;
        Double_t dpTimeBirth  = 0.0;
        Double_t dpTimeDeath  = 0.0;

        dpTree->SetBranchAddress("PID",              &dpPID);
        dpTree->SetBranchAddress("Z",                &dpZ);
        dpTree->SetBranchAddress("A",                &dpA);
        dpTree->SetBranchAddress("CreatorProcess",   dpCreator);   // array: no &
        dpTree->SetBranchAddress("ExcitationEnergy", &dpExcitation);
        dpTree->SetBranchAddress("Weight",           &dpWeight);
        dpTree->SetBranchAddress("IsStable",         &dpIsStable);
        dpTree->SetBranchAddress("TimeBirth",        &dpTimeBirth);
        dpTree->SetBranchAddress("TimeDeath",        &dpTimeDeath);

        TString dpCsvName = TString(stem) + "_DecayProducts.csv";
        std::ofstream dpCsv(dpCsvName.Data());
        dpCsv << "PID,Z,A,CreatorProcess,ExcitationEnergy_MeV,Weight,IsStable,"
                 "TimeBirth_s,TimeDeath_s\n";

        Long64_t dpEntries  = dpTree->GetEntries();
        // Dump the first 10% of rows, capped at 10000
        Long64_t dpCsvLimit = std::min<Long64_t>((dpEntries + 9) / 10, 10000);
        for (Long64_t i = 0; i < dpCsvLimit; ++i) {
            dpTree->GetEntry(i);
            dpCsv << dpPID << ","
                  << dpZ   << ","
                  << dpA   << ","
                  << "\"" << dpCreator << "\","
                  << std::fixed << std::setprecision(6)
                  << dpExcitation << ","
                  << dpWeight     << ","
                  << dpIsStable   << ","
                  << dpTimeBirth * 1e-6 << ",";
            // Stable particles have no meaningful death time; emit "inf".
            if (dpIsStable) dpCsv << "inf\n";
            else             dpCsv << dpTimeDeath * 1e-6 << "\n";
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

// ============================================================
// Multi-file total PHS: reads a run info file, scales each
// file's PHS contribution by P_i/N_i, and sums into a single
// combined pulse-height spectrum.
//
// Usage (in ROOT):
//   .L AnalyzePHS.C
//   analyzePHSTotal("runs.info", "/path/to/rootfiles", "Total", window_us, nBins, eMax)
//
// Arguments:
//   infoPath  - path to the run info file (stem, P_i, N_i per line)
//   rootDir   - directory containing the .root files (default = ".")
//   outStem   - output filename stem for the combined PHS (default = "Total")
//   window_us - charge-collection window length in microseconds (default = 1.0)
//   nBins     - number of histogram bins (default = 3000)
//   eMax      - histogram upper edge in MeV (default = 3.0)
//
// Outputs:
//   <outStem>_PHS_<window_us>us.png  - combined pulse-height spectrum plot
//   <outStem>_PHS_<window_us>us.csv  - bin-center (MeV) and counts
//
// Scale factor logic:
//   f_i = P_i / N_i
//   H_total = sum_i ( f_i * H_i )
//   This correctly handles both the proportional-population case
//   (N_i = c*P_i, so f_i = 1/c for all i) and the equal-population
//   case (N_i = N, so f_i = P_i/N per species).
// ============================================================
void analyzePHSTotal(const char* infoPath,
                     const char* rootDir  = ".",
                     const char* outStem  = "Total",
                     double window_us = 1.0,
                     int    nBins     = 3000,
                     double eMax      = 3.0)
{
    std::vector<RunInfo> runs = parseRunInfo(infoPath);
    if (runs.empty()) {
        std::cerr << "Error: no valid runs found in \"" << infoPath << "\"\n";
        return;
    }

    // Sumw2() ensures correct error propagation through scaling and addition
    TH1D* hTotal = new TH1D("hTotal", "Combined Pulse-Height Spectrum",
                             nBins, 0.0, eMax);
    hTotal->Sumw2();

    std::cout << "\n=== Total PHS Analysis ===\n"
              << "  Info file : " << infoPath << "\n"
              << "  Root dir  : " << rootDir  << "\n"
              << "  Window    : " << window_us << " us\n\n";

    for (auto& r : runs) {
        double scaleFactor = r.P / r.N;

        TString path = TString(rootDir) + "/" + r.file + ".root";
        TFile* f = TFile::Open(path.Data(), "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "Warning: skipping \"" << path << "\" (cannot open)\n";
            continue;
        }

        TTree* tree = dynamic_cast<TTree*>(f->Get("EnergyDeposition"));
        if (!tree) {
            std::cerr << "Warning: skipping \"" << path
                      << "\" (EnergyDeposition tree not found)\n";
            f->Close();
            continue;
        }

        // ---- bind branches ----
        Double_t energyDep = 0.0, weight = 0.0, time = 0.0;
        tree->SetBranchAddress("EnergyDep", &energyDep);
        tree->SetBranchAddress("Weight",    &weight);
        tree->SetBranchAddress("Time",      &time);

        // ---- single-pass accumulation into time windows ----
        std::map<long long, double> windowEnergy;
        std::map<long long, double> windowWeighted;
        std::map<long long, double> windowWeightSum;

        Long64_t nEntries = tree->GetEntries();
        for (Long64_t i = 0; i < nEntries; ++i) {
            tree->GetEntry(i);
            long long idx = static_cast<long long>(std::floor(time / window_us));
            windowEnergy[idx]    += energyDep;
            windowWeighted[idx]  += energyDep * weight;
            windowWeightSum[idx] += weight;
        }

        // ---- build this file's PHS, then scale and accumulate ----
        // Pulse magnitude is the unweighted energy sum; the histogram fill
        // weight is the summed biasing weight of the deposits in the window.
        TH1D* hThis = new TH1D("hThis", "", nBins, 0.0, eMax);
        hThis->Sumw2();
        for (auto& kv : windowEnergy) {
            double E    = kv.second;
            double Wsum = windowWeightSum[kv.first];
            hThis->Fill(E, Wsum);
            // --- previous implementation: energy-weighted average of the weights ---
            // double Wavg = windowWeighted[kv.first] / E;
            // hThis->Fill(E, Wavg);
        }
        hThis->Scale(scaleFactor);
        hTotal->Add(hThis);

        std::cout << "  Processed : " << r.file
                  << "  (P=" << r.P << ", N=" << r.N
                  << ", scale=" << scaleFactor
                  << ", windows=" << windowEnergy.size() << ")\n";

        delete hThis;
        f->Close();
    }

    // ---- output tag ----
    std::ostringstream tagStream;
    tagStream << std::fixed << std::setprecision(2) << window_us;
    TString tag = TString(outStem) + "_PHS_" + tagStream.str().c_str() + "us";

    long long totalWindows = 0;
    double minPulse =  1e30, maxPulse = -1e30;
    for (int b = 1; b <= hTotal->GetNbinsX(); ++b) {
        double val = hTotal->GetBinContent(b);
        if (val <= 0.0) continue;
        ++totalWindows;
        if (val < minPulse) minPulse = val;
        if (val > maxPulse) maxPulse = val;
    }

    std::cout << "\n  Combined bins with content : " << totalWindows << "\n"
              << "  Counts range : [" << minPulse << ", " << maxPulse << "]\n"
              << "==========================\n\n";

    // ---- save PNG ----
    gStyle->SetOptStat(0);
    styleAxis(hTotal, "Pulse Height (MeV)", "Counts");

    TCanvas* c = new TCanvas("cTotal", "Combined Pulse-Height Spectrum", 800, 600);
    c->SetLeftMargin(0.12);
    c->SetBottomMargin(0.12);
    hTotal->Draw("HIST");
    TString pngName = tag + ".png";
    c->SaveAs(pngName.Data());
    std::cout << "Saved plot : " << pngName.Data() << "\n";

    // ---- save CSV ----
    TString csvName = tag + ".csv";
    std::ofstream csv(csvName.Data());
    csv << "BinCenter_MeV,Counts\n";
    for (int b = 1; b <= hTotal->GetNbinsX(); ++b) {
        csv << std::fixed << std::setprecision(6)
            << hTotal->GetBinCenter(b) << ","
            << hTotal->GetBinContent(b) << "\n";
    }
    csv.close();
    std::cout << "Saved data : " << csvName.Data() << "\n";

    // ---- cleanup ----
    delete c;
    delete hTotal;
}
