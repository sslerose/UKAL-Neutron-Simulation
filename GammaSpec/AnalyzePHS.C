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
///
/// Single-run PHS:
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
///   <stem>_GammaEnergy.png        - emitted gamma energy spectrum plot
///   <stem>_GammaEnergy.csv        - bin-center (MeV) and counts
///   <stem>_EmittedParticles.csv   - sampled emitted-particle rows
///   <stem>_DecayProducts.csv      - sampled decay-product rows
///
/// Algorithm:
///   Energy deposit data is organized into time windows indexed by
///   idx = floor(Time / window_us). Within each window, the energy
///   deposits and weights are summed: E[idk] += EnergyDep and
///   W[idk] += Weight. One histogram entry is filled per occupied window at
///   E with pulse height W. Empty windows are skipped.
///
///
/// Multi-run total PHS: reads a run info file and processes PHS and gamma emission
/// in present ROOT files, scaling each file's contributions by P_i/N_i and summing
/// them into combined PHS and gamma emission spectra
///
/// Usage (in ROOT):
///   .L AnalyzePHS.C
///   analyzePHSTotal("runs.info", "/path/to/rootfiles", "Total", window_us, nBins, eMax, toPlot)
///
/// Required arguments:
///   infoPath  - path to the run info file (stem, P_i, N_i per line)
///
/// Optional arguements:
///   rootDir   - directory containing the .root files (default = ".")
///   outStem   - output filename stem for the combined PHS (default = "Total")
///   window_us - charge-collection window length in microseconds (default = 1.0)
///   nBins     - number of histogram bins (default = 3000)
///   eMax      - histogram upper edge in MeV (default = 3.0)
///   toPlot    - also save the per-run PHS and gamma plots/CSVs (default = false)
///
/// Outputs:
///   <outStem>_PHS_<window_us>us.png  - combined pulse-height spectrum plot
///   <outStem>_PHS_<window_us>us.csv  - bin-center (MeV) and counts
///   <outStem>_GammaEnergy.png        - combined gamma emission spectrum plot
///   <outStem>_GammaEnergy.csv        - bin-center (MeV) and counts
///
///   The combined outputs above are always written. When toPlot is true, the
///   per-run <stem>_PHS_<window_us>us.* and <stem>_GammaEnergy.* files are
///   written as well.
///
/// Algorithm:
///   Each .root file in rootDir is passed to the processPHS and processGamma
///   helpers, and their histograms scaled by f_i = P_i / N_i, where P_i is
///   the true initial parent radioisotope population from NeutronActivation
///   and N_i is the parent population simulated in GammaSpec. The total PHS
///   and gamma spectrum are calculated as H_total = sum_i ( f_i * H_i ).
/// ============================================================
//

#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TString.h"
#include "TDatabasePDG.h"
#include "TParticlePDG.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Run metadata for multi-file total PHS analysis
//------------------------------------------------------------------------//
struct RunInfo {
    TString file;  // ROOT file stem (no .root extension)
    double  P;     // True parent population (NeutronActivation)
    double  N;     // Simulated parent population (GammaSpec)
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Parse RunInfo file
//------------------------------------------------------------------------//
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

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Apply consistent axis styling to a histogram
//------------------------------------------------------------------------//
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

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Build pulse-height spectrum from EnergyDeposition tree
//
// Energy deposits are grouped into time windows of length window_us; one
// histogram entry is filled per occupied window at the summed deposit energy,
// weighted by the summed biasing weight of that window.
//
// The returned histogram is owned by the caller and must be deleted by it.
// If toPlot is true, <stem>_PHS_<window_us>us.{png,csv} are also written.
//------------------------------------------------------------------------//
TH1D* processPHS(TTree* tree, const char* stem, double window_us, int nBins, double eMax, bool toPlot)
{
    if (!tree) return nullptr;

    Double_t energyDep = 0.0;
    Double_t weight    = 0.0;
    Double_t time      = 0.0;

    tree->SetBranchAddress("EnergyDep", &energyDep);
    tree->SetBranchAddress("Weight",    &weight);
    tree->SetBranchAddress("Time",      &time);


    //========================================================================//
    // Data processing
    //========================================================================//

    std::map<long long, double> windowEnergy;
    std::map<long long, double> windowWeightSum;

    // Accumulate data into time windows
    Long64_t nEntries = tree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        long long idx = static_cast<long long>(std::floor(time / window_us));
        windowEnergy[idx]    += energyDep;
        windowWeightSum[idx] += weight;
    }

    long long nWindows = static_cast<long long>(windowEnergy.size());

    // Determine energy range of occupied windows for summary
    double minPulse =  1e30;
    double maxPulse = -1e30;
    for (auto& kv : windowEnergy) {
        if (kv.second < minPulse) minPulse = kv.second;
        if (kv.second > maxPulse) maxPulse = kv.second;
    }

    // Print summary
    std::cout << "\n=== Pulse-Height Spectrum Analysis ===\n"
              << "  Run          : " << stem      << "\n"
              << "  Window       : " << window_us << " us\n"
              << "  Rows read    : " << nEntries  << "\n"
              << "  Occupied windows: " << nWindows << "\n"
              << "  Pulse range  : [" << minPulse << ", " << maxPulse << "] MeV\n"
              << "=======================================\n\n";


    //========================================================================//
    // Fill histogram
    //========================================================================//

    gStyle->SetOptStat(0);

    TString hName = TString("hPHS_") + stem;    // Create stem-qualified name
    TH1D* hPHS = new TH1D(hName.Data(), "Pulse-Height Spectrum", nBins, 0.0, eMax);
    hPHS->SetDirectory(nullptr);
    hPHS->Sumw2();  // Proper error propogation

    // Pulse magnitude is unweighted energy sum; fill weight is summed weights of deposits
    for (auto& kv : windowEnergy) {
        double E    = kv.second;
        double Wsum = windowWeightSum[kv.first];
        hPHS->Fill(E, Wsum);
        // --- previous implementation: energy-weighted average of the weights ---
        // double Wavg = windowWeighted[kv.first] / E;
        // hPHS->Fill(E, Wavg);
    }


    //========================================================================//
    // Plot histogram and write CSV if prompted
    //========================================================================//

    styleAxis(hPHS, "Pulse Height (MeV)", "Counts");

    if (toPlot) {
        std::ostringstream tagStream;
        tagStream << std::fixed << std::setprecision(2) << window_us;
        TString tag = TString(stem) + "_PHS_" + tagStream.str().c_str() + "us";

        // Save histogram
        TCanvas* c = new TCanvas("cPHS", "Pulse-Height Spectrum", 800, 600);
        c->SetLeftMargin(0.12);
        c->SetBottomMargin(0.12);
        hPHS->Draw("HIST");
        TString pngName = tag + ".png";
        c->SaveAs(pngName.Data());
        std::cout << "Saved plot : " << pngName.Data() << "\n";

        // Save CSV
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

        delete c;
    }

    return hPHS;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Build emitted-gamma energy spectrum from EmittedParticles tree
//------------------------------------------------------------------------//
TH1D* processGamma(TTree* emTree,
                   const char* stem,
                   int    nBins,
                   double eMax,
                   bool   toPlot)
{
    if (!emTree) return nullptr;

    Int_t    emPID    = 0;
    Double_t emEnergy = 0.0;

    emTree->SetBranchAddress("PID",    &emPID);
    emTree->SetBranchAddress("Energy", &emEnergy);


    //========================================================================//
    // Fill Histogram
    //========================================================================//

    gStyle->SetOptStat(0);

    TString hName = TString("hGammaE_") + stem; // Stem-qualified name
    TH1D* hGammaE = new TH1D(hName.Data(), "Emitted Gamma Energy Spectrum", nBins, 0.0, eMax);
    hGammaE->SetDirectory(nullptr);
    hGammaE->Sumw2();   // Proper error propogation

    Long64_t emEntries = emTree->GetEntries();
    for (Long64_t i = 0; i < emEntries; ++i) {
        emTree->GetEntry(i);
        if (emPID == 22) {
            hGammaE->Fill(emEnergy);
        }
    }


    //========================================================================//
    // Plot histogram and write CSV if prompted
    //========================================================================//

    styleAxis(hGammaE, "Energy (MeV)", "Counts");

    if (toPlot) {
        TString tag = TString(stem) + "_GammaEnergy";

        // Save histogram
        TCanvas* cGammaE = new TCanvas("cGammaE", "Emitted Gamma Energy", 800, 600);
        cGammaE->SetLeftMargin(0.12);
        cGammaE->SetBottomMargin(0.12);
        hGammaE->Draw("HIST");
        TString gammaEPng = tag + ".png";
        cGammaE->SaveAs(gammaEPng.Data());
        std::cout << "Saved plot : " << gammaEPng.Data() << "\n";

        // Save CSV
        TString gammaECsv = tag + ".csv";
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
    }

    return hGammaE;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Single-file analysis
//------------------------------------------------------------------------//
void analyzePHS(const char* filePath,
                double window_us = 1.0,
                int    nBins     = 3000,
                double eMax      = 3.0)
{
    // Derive output filename stem
    TString inputPath(filePath);
    TString stem = inputPath;
    int lastSlash = stem.Last('/');
    if (lastSlash >= 0) stem = stem(lastSlash + 1, stem.Length() - lastSlash - 1);
    if (stem.EndsWith(".root")) stem.Remove(stem.Length() - 5, 5);

    // Open file
    TFile* f = TFile::Open(filePath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error: cannot open \"" << filePath << "\"\n";
        return;
    }


    //========================================================================//
    // Pulse-height spectrum
    //========================================================================//

    TTree* tree = dynamic_cast<TTree*>(f->Get("EnergyDeposition"));

    if (!tree) {
        std::cerr << "Error: \"EnergyDeposition\" tree not found in \"" << filePath << "\"\n";
        f->Close();
        return;
    }

    // Build, plot, and save PHS
    TH1D* hPHS = processPHS(tree, stem.Data(), window_us, nBins, eMax, true);


    //========================================================================//
    // EmittedParticles CSV and gamma spectrum
    //========================================================================//

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
        Long64_t emCsvLimit = std::min<Long64_t>((emEntries + 9) / 10, 10000);  // Limit CSV to first 10% of rows, capped at 10000

        for (Long64_t i = 0; i < emCsvLimit; ++i) {
            emTree->GetEntry(i);
            TParticlePDG* pdgParticle = pdgDB->GetParticle(emPID);
            const char* emName = pdgParticle ? pdgParticle->GetName() : "unknown";
            emCsv << emPID << ","
                  << emName << ","
                  << std::fixed << std::setprecision(6)
                  << emEnergy << ","
                  << emWeight << ","
                  << emTime * 1e-6 << "\n";
        }
        emCsv.close();
        std::cout << "Saved data : " << emCsvName.Data()
                  << "  (first " << emCsvLimit << " of " << emEntries << " rows)\n";

        // Build, plot, and save the emitted gamma energy spectrum
        TH1D* hGammaE = processGamma(emTree, stem.Data(), nBins, eMax, true);
        delete hGammaE;
    }


    //========================================================================//
    // DecayProducts CSV
    //========================================================================//

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

    // Cleanup
    delete hPHS;
    f->Close();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

//------------------------------------------------------------------------//
// Multi-file analysis
//------------------------------------------------------------------------//
void analyzePHSTotal(const char* infoPath, const char* rootDir = ".", const char* outStem = "Total", double window_us = 1.0, int nBins = 3000, double eMax = 3.0, bool toPlot = false)
{
    std::vector<RunInfo> runs = parseRunInfo(infoPath);
    if (runs.empty()) {
        std::cerr << "Error: no valid runs found in \"" << infoPath << "\"\n";
        return;
    }

    // Create total PHS
    TH1D* hTotal = new TH1D("hTotal", "Combined Pulse-Height Spectrum", nBins, 0.0, eMax);
    hTotal->SetDirectory(nullptr);
    hTotal->Sumw2();    // Proper error propogation

    // Create total gamma spectrum
    TH1D* hGammaTotal = new TH1D("hGammaTotal", "Combined Emitted Gamma Energy Spectrum", nBins, 0.0, eMax);
    hGammaTotal->SetDirectory(nullptr);
    hGammaTotal->Sumw2();   // Proper error propogation

    std::cout << "\n=== Total PHS Analysis ===\n"
              << "  Info file : " << infoPath << "\n"
              << "  Root dir  : " << rootDir  << "\n"
              << "  Window    : " << window_us << " us\n\n";


    //========================================================================//
    // Loop through available runs and fill histograms
    //========================================================================//

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

        // Build this file's PHS, then scale and accumulate
        TH1D* hThis = processPHS(tree, r.file.Data(), window_us, nBins, eMax, toPlot);
        hThis->Scale(scaleFactor);
        hTotal->Add(hThis);

        std::cout << "  Processed : " << r.file
                  << "  (P=" << r.P << ", N=" << r.N
                  << ", scale=" << scaleFactor << ")\n";

        delete hThis;

        TTree* emTree = dynamic_cast<TTree*>(f->Get("EmittedParticles"));

        // Build this file's gamma spectrum, then scale and accumulate
        if (!emTree) {
            std::cerr << "Warning: \"" << path
                      << "\" (EmittedParticles tree not found), "
                      << "no gamma contribution from this run\n";
        } else {
            TH1D* hGammaThis = processGamma(emTree, r.file.Data(), nBins, eMax, toPlot);
            hGammaThis->Scale(scaleFactor);
            hGammaTotal->Add(hGammaThis);
            delete hGammaThis;
        }

        f->Close();
    }

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


    //========================================================================//
    // Combined PHS
    //========================================================================//

    // PHS output tag
    std::ostringstream tagStream;
    tagStream << std::fixed << std::setprecision(2) << window_us;
    TString tag = TString(outStem) + "_PHS_" + tagStream.str().c_str() + "us";

    // Save PHS histogram
    gStyle->SetOptStat(0);
    styleAxis(hTotal, "Pulse Height (MeV)", "Counts");

    TCanvas* c = new TCanvas("cTotal", "Combined Pulse-Height Spectrum", 800, 600);
    c->SetLeftMargin(0.12);
    c->SetBottomMargin(0.12);
    hTotal->Draw("HIST");
    TString pngName = tag + ".png";
    c->SaveAs(pngName.Data());
    std::cout << "Saved plot : " << pngName.Data() << "\n";

    // Save PHS CSV
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


    //========================================================================//
    // Combined gamma spectrum
    //========================================================================//

    // Gamma spectrum output tag
    TString gammaTag = TString(outStem) + "_GammaEnergy";
    styleAxis(hGammaTotal, "Energy (MeV)", "Counts");

    // Save gamma spectrum
    TCanvas* cGamma = new TCanvas("cGammaTotal",
                                  "Combined Emitted Gamma Energy Spectrum", 800, 600);
    cGamma->SetLeftMargin(0.12);
    cGamma->SetBottomMargin(0.12);
    hGammaTotal->Draw("HIST");
    TString gammaPng = gammaTag + ".png";
    cGamma->SaveAs(gammaPng.Data());
    std::cout << "Saved plot : " << gammaPng.Data() << "\n";

    // Save gamma spectrum CSV
    TString gammaCsvName = gammaTag + ".csv";
    std::ofstream gammaCsv(gammaCsvName.Data());
    gammaCsv << "BinCenter_MeV,Counts\n";
    for (int b = 1; b <= hGammaTotal->GetNbinsX(); ++b) {
        gammaCsv << std::fixed << std::setprecision(6)
                 << hGammaTotal->GetBinCenter(b) << ","
                 << hGammaTotal->GetBinContent(b) << "\n";
    }
    gammaCsv.close();
    std::cout << "Saved data : " << gammaCsvName.Data() << "\n";

    // Cleanup
    delete c;
    delete cGamma;
    delete hTotal;
    delete hGammaTotal;
}
