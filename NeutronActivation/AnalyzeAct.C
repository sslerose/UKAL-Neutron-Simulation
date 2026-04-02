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
/// \file AnalyzeAct.C
/// \brief ROOT macro for analyzing NeutronActivation simulation output.
///
/// Usage (in ROOT):
///   .L AnalyzeAct.C
///   analyzeAct("path/to/data.root", beamTime, "timeScale", coolingTime)
///
/// Required arguments:
///   filePath   - path to the .root data file
///   beamTime   - beam-on duration (in the chosen time scale)
///   timeScale  - one of: s/sec/seconds, m/min/minutes, h/hr/hours
///
/// Optional argument:
///   coolingTime - post-beam cooling time (default = 0, in chosen time scale)
///
/// Outputs:
///   CaptureTimes.png           - histogram of neutron capture times
///   Population_<ion>.png       - population vs. time step plot per species
///   surviving_populations.txt  - population of each species at t = tEnd
///
/// Diagnostic:
///   printSpeciesInfo("path/to/data.root") - print a summary of all ion
///   species found in PopulationData (entry counts, birth/death time ranges)
//

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TString.h"
#include "TMath.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

// ============================================================
// Helper: apply consistent axis styling to a histogram
// ============================================================
void styleAxis(TH1D* h, const char* xTitle, const char* yTitle) {
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
// Helper: apply consistent axis styling to a TGraph
// ============================================================
void styleAxis(TGraph* g, const char* xTitle, const char* yTitle) {
    g->GetXaxis()->SetTitle(xTitle);
    g->GetYaxis()->SetTitle(yTitle);
    g->GetXaxis()->SetTitleOffset(1.3);
    g->GetYaxis()->SetTitleOffset(1.5);
    g->GetXaxis()->SetLabelSize(0.04);
    g->GetYaxis()->SetLabelSize(0.04);
    g->GetXaxis()->SetTitleSize(0.04);
    g->GetYaxis()->SetTitleSize(0.04);
    g->GetXaxis()->CenterTitle(true);
    g->GetYaxis()->CenterTitle(true);
}

// ============================================================
// Diagnostic: print a summary of all ion species in PopulationData
// ============================================================
void printSpeciesInfo(const char* filePath)
{
    TFile* f = TFile::Open(filePath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error: cannot open file \"" << filePath << "\"\n";
        return;
    }

    TTree* popTree = dynamic_cast<TTree*>(f->Get("PopulationData"));
    if (!popTree) {
        std::cerr << "Error: \"PopulationData\" tree not found.\n";
        f->Close();
        return;
    }

    // Read string branch as char array (ROOT stores SColumn as Char_t[])
    char     ionName[256]  = {};
    Double_t timeBirth     = 0.0;
    Double_t timeDeath     = 0.0;
    Double_t weight        = 0.0;

    popTree->SetBranchAddress("IonName",   ionName);
    popTree->SetBranchAddress("TimeBirth", &timeBirth);
    popTree->SetBranchAddress("TimeDeath", &timeDeath);
    popTree->SetBranchAddress("Weight",    &weight);

    // Per-species: count, min/max birth, min/max death, total weight
    struct SpeciesStat {
        Long64_t count     = 0;
        Double_t minBirth  =  1e300;
        Double_t maxBirth  = -1e300;
        Double_t minDeath  =  1e300;
        Double_t maxDeath  = -1e300;
        Double_t sumWeight = 0.0;
    };
    std::map<std::string, SpeciesStat> stats;

    Long64_t nEntries = popTree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        popTree->GetEntry(i);
        std::string name(ionName);
        auto& s = stats[name];
        s.count++;
        if (timeBirth < s.minBirth) s.minBirth = timeBirth;
        if (timeBirth > s.maxBirth) s.maxBirth = timeBirth;
        if (timeDeath < s.minDeath) s.minDeath = timeDeath;
        if (timeDeath > s.maxDeath) s.maxDeath = timeDeath;
        s.sumWeight += weight;
    }

    std::cout << "\n=== printSpeciesInfo: " << filePath << " ===\n"
              << "  Total PopulationData entries: " << nEntries << "\n"
              << "  Unique species found        : " << stats.size() << "\n\n";

    // Header
    std::cout << std::left
              << std::setw(24) << "Ion Species"
              << std::right
              << std::setw(10) << "Entries"
              << std::setw(16) << "Birth min (s)"
              << std::setw(16) << "Birth max (s)"
              << std::setw(16) << "Death min (s)"
              << std::setw(16) << "Death max (s)"
              << std::setw(14) << "Sum Weight"
              << "\n";
    std::cout << std::string(112, '-') << "\n";

    for (const auto& kv : stats) {
        const auto& s = kv.second;
        std::cout << std::left  << std::setw(24) << kv.first
                  << std::right << std::fixed << std::setprecision(4)
                  << std::setw(10) << s.count
                  << std::setw(16) << s.minBirth
                  << std::setw(16) << s.maxBirth
                  << std::setw(16) << s.minDeath
                  << std::setw(16) << s.maxDeath
                  << std::setw(14) << s.sumWeight
                  << "\n";
    }
    std::cout << std::string(112, '-') << "\n\n";

    f->Close();
    delete f;
}

// ============================================================
// Main analysis function
// ============================================================
void analyzeAct(const char* filePath,
                Double_t     beamTime,
                const char*  timeScale,
                Double_t     coolingTime = 0.0)
{
    // ----------------------------------------------------------
    // 1. Parse time scale
    // ----------------------------------------------------------
    Double_t timeScaleFactor = 1.0;
    std::string timeUnit     = "s";

    std::string ts(timeScale);
    // Convert to lowercase for case-insensitive comparison
    std::transform(ts.begin(), ts.end(), ts.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (ts == "s" || ts == "sec" || ts == "seconds") {
        timeScaleFactor = 1.0;
        timeUnit        = "s";
    } else if (ts == "m" || ts == "min" || ts == "minutes") {
        timeScaleFactor = 60.0;
        timeUnit        = "min";
    } else if (ts == "h" || ts == "hr" || ts == "hours") {
        timeScaleFactor = 3600.0;
        timeUnit        = "hr";
    } else {
        std::cerr << "\nError: unrecognized time scale \"" << timeScale << "\".\n"
                  << "Accepted values: s, sec, seconds, m, min, minutes, h, hr, hours\n\n";
        return;
    }

    // ----------------------------------------------------------
    // 2. Derived time limits (all in display units)
    // ----------------------------------------------------------
    Double_t tBeam = beamTime;
    Double_t tCool = coolingTime;
    Double_t tEnd  = tBeam + tCool;
    Double_t tMax  = 1.2 * tEnd;

    std::cout << "\n=== analyzeAct ===\n"
              << "  File        : " << filePath   << "\n"
              << "  Beam time   : " << tBeam      << " " << timeUnit << "\n"
              << "  Cooling time: " << tCool      << " " << timeUnit << "\n"
              << "  t_end       : " << tEnd       << " " << timeUnit << "\n"
              << "  t_max (axis): " << tMax       << " " << timeUnit << "\n\n";

    // ----------------------------------------------------------
    // 3. Open ROOT file
    // ----------------------------------------------------------
    TFile* f = TFile::Open(filePath, "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error: cannot open file \"" << filePath << "\"\n";
        return;
    }

    // ----------------------------------------------------------
    // 4. Global style
    // ----------------------------------------------------------
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(1);
    gStyle->SetTitleFontSize(0.045);
    gStyle->SetFrameLineWidth(1);

    // ============================================================
    // SECTION A: Detector data — capture time histogram
    // ============================================================
    TTree* detTree = dynamic_cast<TTree*>(f->Get("DetectorData"));
    if (!detTree) {
        std::cerr << "Error: \"DetectorData\" tree not found in file.\n";
        f->Close();
        return;
    }

    Int_t    captureFlag = 0;
    Double_t captureTime = 0.0;
    detTree->SetBranchAddress("CaptureFlag", &captureFlag);
    detTree->SetBranchAddress("CaptureTime", &captureTime);

    // Build histogram with 200 bins over [0, tMax]
    std::ostringstream hName;
    hName << "hCaptureTimes";
    TH1D* hCapture = new TH1D(hName.str().c_str(),
                               "Neutron Capture Time Distribution",
                               200, 0.0, tMax);

    Long64_t nDetEntries = detTree->GetEntries();
    for (Long64_t i = 0; i < nDetEntries; ++i) {
        detTree->GetEntry(i);
        if (captureFlag == 1 && captureTime >= 0.0) {
            hCapture->Fill(captureTime / timeScaleFactor);
        }
    }

    // Style and draw
    hCapture->SetLineColor(kBlue + 1);
    hCapture->SetLineWidth(2);
    hCapture->SetFillColor(kBlue - 9);
    hCapture->SetFillStyle(1001);

    std::string xLabelCapture = "Capture Time [" + timeUnit + "]";
    styleAxis(hCapture, xLabelCapture.c_str(), "Counts");

    TCanvas* cCapture = new TCanvas("cCapture", "Capture Time Distribution", 800, 600);
    cCapture->SetGrid();
    cCapture->SetLeftMargin(0.13);
    cCapture->SetRightMargin(0.05);
    cCapture->SetBottomMargin(0.12);

    hCapture->Draw("HIST");
    cCapture->Update();
    cCapture->SaveAs("CaptureTimes.png");
    std::cout << "Saved: CaptureTimes.png\n";

    // ============================================================
    // SECTION B: Population data — per-species step plots
    // ============================================================
    TTree* popTree = dynamic_cast<TTree*>(f->Get("PopulationData"));
    if (!popTree) {
        std::cerr << "Error: \"PopulationData\" tree not found in file.\n";
        f->Close();
        return;
    }

    // Branch variables
    // NOTE: Geant4's CreateNtupleSColumn writes strings as Char_t[] in ROOT.
    //       Use a char array and pass the raw pointer (no &) to SetBranchAddress.
    char     ionName[256] = {};
    Double_t timeBirth    = 0.0;
    Double_t timeDeath    = 0.0;
    Double_t weight       = 0.0;

    popTree->SetBranchAddress("IonName",   ionName);   // raw array — no &
    popTree->SetBranchAddress("TimeBirth", &timeBirth);
    popTree->SetBranchAddress("TimeDeath", &timeDeath);
    popTree->SetBranchAddress("Weight",    &weight);

    // Collect rows grouped by species: map<name, vector<(birth, death, weight)>>
    std::map<std::string, std::vector<std::tuple<Double_t, Double_t, Double_t>>> speciesData;

    Long64_t nPopEntries = popTree->GetEntries();
    for (Long64_t i = 0; i < nPopEntries; ++i) {
        popTree->GetEntry(i);
        speciesData[std::string(ionName)].emplace_back(timeBirth, timeDeath, weight);
    }

    std::cout << "Found " << speciesData.size() << " ion species.\n\n";

    // Prepare output file for surviving populations
    std::ofstream outFile("surviving_populations.txt");
    using PrintFn = std::function<void(const std::string&)>;
    PrintFn printBoth = [&outFile](const std::string& msg) {
        std::cout << msg;
        if (outFile.is_open()) outFile << msg;
    };

    // Write header
    {
        std::ostringstream hdr;
        hdr << "=========================================\n"
            << " Surviving Ion Populations at t = tEnd\n"
            << "=========================================\n"
            << "  Beam time   : " << std::fixed << std::setprecision(4)
            << tBeam << " " << timeUnit << "\n"
            << "  Cooling time: " << tCool << " " << timeUnit << "\n"
            << "  t_end       : " << tEnd  << " " << timeUnit << "\n"
            << "-----------------------------------------\n"
            << std::left << std::setw(24) << "Ion Species"
            << std::right << std::setw(20) << "Population at t_end\n"
            << "-----------------------------------------\n";
        printBoth(hdr.str());
    }

    // Per-species step plot + surviving population lookup
    for (const auto& kv : speciesData) {
        const std::string& species = kv.first;
        const auto& rows = kv.second;

        // Build event list: (time_display, delta_weight)
        std::vector<std::pair<Double_t, Double_t>> events;
        events.reserve(rows.size() * 2);
        for (const auto& row : rows) {
            Double_t tb = std::get<0>(row) / timeScaleFactor;
            Double_t td = std::get<1>(row) / timeScaleFactor;
            Double_t w  = std::get<2>(row);
            events.emplace_back(tb, +w);
            events.emplace_back(td, -w);
        }

        // Sort by time
        std::sort(events.begin(), events.end(),
                  [](const std::pair<Double_t,Double_t>& a,
                     const std::pair<Double_t,Double_t>& b){
                      return a.first < b.first;
                  });

        // Build cumulative population arrays (step representation)
        // For a true step appearance we insert two points per event:
        //   (t, popBefore) then (t, popAfter)
        std::vector<Double_t> grTimes;
        std::vector<Double_t> grPops;
        grTimes.push_back(0.0);
        grPops.push_back(0.0);

        Double_t cumPop = 0.0;
        for (const auto& ev : events) {
            Double_t t     = ev.first;
            Double_t delta = ev.second;
            // Hold the old value up to this time
            grTimes.push_back(t);
            grPops.push_back(cumPop);
            // Then jump to new value
            cumPop += delta;
            grTimes.push_back(t);
            grPops.push_back(cumPop);
        }

        // Surviving population: last grPops value at grTimes <= tEnd
        Double_t survivingPop = 0.0;
        for (std::size_t k = 0; k < grTimes.size(); ++k) {
            if (grTimes[k] <= tEnd) {
                survivingPop = grPops[k];
            }
        }

        // Write to text file
        {
            std::ostringstream row;
            row << std::left  << std::setw(24) << species
                << std::right << std::setw(20) << std::fixed << std::setprecision(4)
                << survivingPop << "\n";
            printBoth(row.str());
        }

        // Determine y-axis range
        Double_t yMax = 0.0;
        for (Double_t p : grPops) { if (p > yMax) yMax = p; }
        if (yMax <= 0.0) yMax = 1.0;
        Double_t yPad = 1.15 * yMax;

        // Create TGraph
        Int_t nPts = static_cast<Int_t>(grTimes.size());
        TGraph* gPop = new TGraph(nPts, grTimes.data(), grPops.data());

        std::string graphTitle = "Population: " + species;
        gPop->SetTitle(graphTitle.c_str());
        gPop->SetLineColor(kRed + 1);
        gPop->SetLineWidth(2);

        // Canvas
        std::string canvName = "cPop_" + species;
        TCanvas* cPop = new TCanvas(canvName.c_str(), graphTitle.c_str(), 800, 600);
        cPop->SetGrid();
        cPop->SetLeftMargin(0.13);
        cPop->SetRightMargin(0.05);
        cPop->SetBottomMargin(0.12);

        // Draw graph with axis frame
        gPop->Draw("AL");
        gPop->GetXaxis()->SetLimits(0.0, tMax);
        gPop->SetMinimum(0.0);
        gPop->SetMaximum(yPad);

        std::string xLabelPop = "Time [" + timeUnit + "]";
        styleAxis(gPop, xLabelPop.c_str(), "Population");
        cPop->Update();

        // Vertical dashed line at tBeam
        TLine* lBeam = new TLine(tBeam, 0.0, tBeam, yPad);
        lBeam->SetLineStyle(2);
        lBeam->SetLineColor(kGray + 2);
        lBeam->SetLineWidth(2);
        lBeam->Draw("SAME");

        // Vertical dashed line at tEnd (only if cooling time is non-zero)
        TLine* lEnd = nullptr;
        if (tCool > 0.0) {
            lEnd = new TLine(tEnd, 0.0, tEnd, yPad);
            lEnd->SetLineStyle(2);
            lEnd->SetLineColor(kGreen + 2);
            lEnd->SetLineWidth(2);
            lEnd->Draw("SAME");
        }

        // Legend for dashed lines
        TLegend* leg = new TLegend(0.55, 0.75, 0.93, 0.90);
        leg->AddEntry(gPop,  ("Population: " + species).c_str(), "l");
        leg->AddEntry(lBeam, ("Beam end (t = " + std::to_string(tBeam).substr(0,6)
                               + " " + timeUnit + ")").c_str(), "l");
        if (lEnd) {
            leg->AddEntry(lEnd, ("Cool end (t = " + std::to_string(tEnd).substr(0,6)
                                  + " " + timeUnit + ")").c_str(), "l");
        }
        leg->SetBorderSize(0);
        leg->SetTextSize(0.03);
        leg->Draw();

        cPop->Update();

        // Build a safe filename (replace characters that may be problematic)
        std::string safeName = species;
        std::replace(safeName.begin(), safeName.end(), '/', '_');
        std::replace(safeName.begin(), safeName.end(), ' ', '_');
        std::string outPng = "Population_" + safeName + ".png";
        cPop->SaveAs(outPng.c_str());
        std::cout << "Saved: " << outPng << "\n";

        // Cleanup per-iteration heap objects
        delete gPop;
        delete lBeam;
        if (lEnd) delete lEnd;
        delete leg;
        delete cPop;
    }

    // Close text file and ROOT file
    {
        std::ostringstream footer;
        footer << "-----------------------------------------\n"
               << " (population is the last recorded value\n"
               << "  at or before t_end)\n"
               << "=========================================\n";
        printBoth(footer.str());
    }
    if (outFile.is_open()) outFile.close();
    std::cout << "\nSaved: surviving_populations.txt\n";

    // Cleanup
    delete hCapture;
    delete cCapture;
    f->Close();
    delete f;

    std::cout << "\n=== analyzeAct complete ===\n\n";
}
