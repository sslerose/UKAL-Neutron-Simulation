//
// AnalyzeEff.C
// ROOT analysis macro for neutron detection efficiency measurements
//
// Processes mono-energetic gun runs (1-150 keV) to determine energy-dependent
// detection efficiency ε(E) = captures / entries. Fits a polynomial to the
// efficiency data and exports fit parameters for use by AnalyzeData.C.
//
// Usage:
//   root -l
//   .L AnalyzeEff.C
//   analyzeEfficiency("path/to/efficiency/data")
//
//   // Or with default current directory:
//   analyzeEfficiency()
//
// Compatible with ROOT 6.14+
//

#include "TFile.h"
#include "TTree.h"
#include "TChain.h"
#include "TH1D.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TString.h"
#include "TAxis.h"
#include "TMath.h"
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <sstream>
#include <cmath>

//============================================================================//
// Structure to hold parsed filename parameters
//============================================================================//
struct FilenameParams {
    Double_t angle;
    Double_t distance;
    Bool_t valid;

    FilenameParams() : angle(-999.0), distance(-999.0), valid(false) {}
};

//============================================================================//
// Parse detector angle and distance from filename
// Expected format:
//   nTOF_Gun_50_0keV_0_0deg_25_0cm.root  ->  angle=0.0, distance=25.0
//============================================================================//
FilenameParams parseFilenameParams(const TString& filename) {

    FilenameParams params;

    // First, parse distance (find "cm" and work backwards)
    Int_t cmPos = filename.Index("cm");
    if (cmPos == -1) {
        std::cerr << "Warning: 'cm' not found in filename: " << filename << std::endl;
        return params;
    }

    // Extract everything before "cm"
    TString beforeCm = filename(0, cmPos);

    // Find last underscore before "cm" (separates decimal part of distance)
    Int_t lastUnderscoreCm = beforeCm.Last('_');
    if (lastUnderscoreCm == -1) {
        std::cerr << "Warning: Cannot parse distance from: " << filename << std::endl;
        return params;
    }

    // Extract distance decimal part
    TString distDecPart = beforeCm(lastUnderscoreCm + 1, beforeCm.Length() - lastUnderscoreCm - 1);

    // Find second-to-last underscore before "cm" (before distance integer part)
    TString beforeDistDec = beforeCm(0, lastUnderscoreCm);
    Int_t secondLastUnderscoreCm = beforeDistDec.Last('_');
    if (secondLastUnderscoreCm == -1) {
        std::cerr << "Warning: Cannot parse distance from: " << filename << std::endl;
        return params;
    }

    // Extract distance integer part
    TString distIntPart = beforeDistDec(secondLastUnderscoreCm + 1, lastUnderscoreCm - secondLastUnderscoreCm - 1);
    TString distStr = distIntPart + "." + distDecPart;
    params.distance = distStr.Atof();

    // Now parse angle from the portion before the distance
    Int_t degPos = beforeDistDec.Index("deg");
    if (degPos == -1) {
        std::cerr << "Warning: 'deg' not found in filename: " << filename << std::endl;
        return params;
    }

    // Extract everything before "deg"
    TString beforeDeg = beforeDistDec(0, degPos);

    // Find last underscore before "deg" (separates decimal part of angle)
    Int_t lastUnderscoreDeg = beforeDeg.Last('_');
    if (lastUnderscoreDeg == -1) {
        std::cerr << "Warning: Cannot parse angle from: " << filename << std::endl;
        return params;
    }

    // Extract angle decimal part
    TString angleDecPart = beforeDeg(lastUnderscoreDeg + 1, beforeDeg.Length() - lastUnderscoreDeg - 1);

    // Find second-to-last underscore before "deg" (before angle integer part)
    TString beforeAngleDec = beforeDeg(0, lastUnderscoreDeg);
    Int_t secondLastUnderscoreDeg = beforeAngleDec.Last('_');
    if (secondLastUnderscoreDeg == -1) {
        std::cerr << "Warning: Cannot parse angle from: " << filename << std::endl;
        return params;
    }

    // Extract angle integer part (may include negative sign)
    TString angleIntPart = beforeDeg(secondLastUnderscoreDeg + 1, lastUnderscoreDeg - secondLastUnderscoreDeg - 1);
    TString angleStr = angleIntPart + "." + angleDecPart;
    params.angle = angleStr.Atof();

    params.valid = true;
    return params;
}

//============================================================================//
// Parse beam energy from filename
// Expected format:
//   nTOF_Gun_50_0keV_0_0deg_25_0cm.root  ->  energy=50.0
//   nTOF_Gun_1_0keV_0_0deg_25_0cm.root   ->  energy=1.0
//============================================================================//
Double_t parseEnergy(const TString& filename) {

    Int_t kevPos = filename.Index("keV");
    if (kevPos == -1) {
        std::cerr << "Warning: 'keV' not found in filename: " << filename << std::endl;
        return -1.0;
    }

    // Extract everything before "keV"
    TString beforeKeV = filename(0, kevPos);

    // Find last underscore before "keV" (separates decimal part of energy)
    Int_t lastUnderscore = beforeKeV.Last('_');
    if (lastUnderscore == -1) {
        std::cerr << "Warning: Cannot parse energy from: " << filename << std::endl;
        return -1.0;
    }

    // Extract energy decimal part
    TString decPart = beforeKeV(lastUnderscore + 1, beforeKeV.Length() - lastUnderscore - 1);

    // Find second-to-last underscore (before energy integer part)
    TString beforeDec = beforeKeV(0, lastUnderscore);
    Int_t secondLastUnderscore = beforeDec.Last('_');
    if (secondLastUnderscore == -1) {
        std::cerr << "Warning: Cannot parse energy from: " << filename << std::endl;
        return -1.0;
    }

    // Extract energy integer part
    TString intPart = beforeDec(secondLastUnderscore + 1, lastUnderscore - secondLastUnderscore - 1);
    TString energyStr = intPart + "." + decPart;
    return energyStr.Atof();
}

//============================================================================//
// Result structure for per-energy analysis
//============================================================================//
struct EnergyResults {
    TString filename;
    Double_t energy;              // Mono-energetic gun energy (keV)
    Double_t angle;               // Detector angle (should be 0.0)
    Double_t distance;            // Detector distance (cm)
    ULong64_t totalEvents;
    ULong64_t entries;            // Neutrons that entered the detector
    ULong64_t captures;           // Neutrons captured
    Double_t efficiency;          // captures / entries
    Double_t efficiencyError;     // Binomial error

    // TOF energy statistics
    Double_t meanTOFEnergy;
    Double_t stddevTOFEnergy;
    Double_t stderrTOFEnergy;

    // Generated neutron energy statistics (for neutrons that entered detector)
    Double_t meanGenEnergy;
    Double_t stddevGenEnergy;
    Double_t stderrGenEnergy;

    // Energy ratio statistics (TOF / Generated for captured neutrons)
    Double_t meanEnergyRatio;

    EnergyResults()
        : energy(-999.0), angle(-999.0), distance(-999.0),
          totalEvents(0), entries(0), captures(0),
          efficiency(0.0), efficiencyError(0.0),
          meanTOFEnergy(0.0), stddevTOFEnergy(0.0), stderrTOFEnergy(0.0),
          meanGenEnergy(0.0), stddevGenEnergy(0.0), stderrGenEnergy(0.0),
          meanEnergyRatio(0.0) {}
};

//============================================================================//
// Analyze a single energy point (one or more files at the same energy)
//============================================================================//
EnergyResults analyzeEnergyPoint(Double_t energy, Double_t angle, Double_t distance,
                                  const std::vector<TString>& files) {

    EnergyResults result;
    result.energy = energy;
    result.angle = angle;
    result.distance = distance;

    if (files.empty()) return result;

    result.filename = files[0];

    // Create TChain for multi-file processing
    TChain chain("DetectorData");
    for (const auto& file : files) {
        chain.Add(file);
    }

    // Create RDataFrame from chain
    ROOT::RDataFrame df(chain);

    // Count total events
    auto countTotal = df.Count();

    //========================================================================//
    // Filter for neutrons that entered the detector (EntryFlag == 1)
    //========================================================================//
    auto dfEntered = df.Filter("EntryFlag == 1");
    auto countEntries = dfEntered.Count();
    auto meanGenE = dfEntered.Mean("NeutronEnergy");
    auto stddevGenE = dfEntered.StdDev("NeutronEnergy");

    //========================================================================//
    // Filter for captures (CaptureFlag == 1 with valid TOF energy)
    //========================================================================//
    auto dfCapture = df.Filter("CaptureFlag == 1 && TOFEnergy > 0");
    auto countCaptures = dfCapture.Count();
    auto meanTOF = dfCapture.Mean("TOFEnergy");
    auto stddevTOF = dfCapture.StdDev("TOFEnergy");

    // Energy ratio for captured neutrons
    auto dfCaptureWithRatio = dfCapture.Define("EnergyRatio",
        "NeutronEnergy > 0 ? TOFEnergy / NeutronEnergy : -1.0");
    auto histEnergyRatio = dfCaptureWithRatio.Filter("EnergyRatio > 0").Histo1D(
        {"hEnergyRatio_tmp", "EnergyRatio", 200, 0, 3}, "EnergyRatio");

    //========================================================================//
    // Trigger computation
    //========================================================================//
    result.totalEvents = *countTotal;
    result.entries = *countEntries;
    result.captures = *countCaptures;
    result.meanTOFEnergy = *meanTOF;
    result.stddevTOFEnergy = *stddevTOF;

    if (result.entries > 0) {
        result.meanGenEnergy = *meanGenE;
        result.stddevGenEnergy = *stddevGenE;
        if (result.entries > 1) {
            result.stderrGenEnergy = result.stddevGenEnergy / TMath::Sqrt(result.entries);
        }
    }

    // Calculate efficiency: captures / entries (with binomial error)
    if (result.entries > 0) {
        result.efficiency = (Double_t)result.captures / result.entries;
        result.efficiencyError = TMath::Sqrt(result.efficiency * (1.0 - result.efficiency) / result.entries);
    }

    // Standard error of mean for TOF
    if (result.captures > 1) {
        result.stderrTOFEnergy = result.stddevTOFEnergy / TMath::Sqrt(result.captures);
    }

    // Mean energy ratio
    if (result.captures > 0) {
        result.meanEnergyRatio = histEnergyRatio->GetMean();
    }

    return result;
}

//============================================================================//
// Main analysis function
//============================================================================//
void analyzeEfficiency(const char* directory = ".", const char* pattern = "nTOF_Gun_", int nThreads = 8) {

    // Open output file for saving results
    std::ofstream outFile("eff_analysis_results.txt");
    if (!outFile.is_open()) {
        std::cerr << "Warning: Could not open eff_analysis_results.txt for writing. Results will only print to console.\n";
    }

    // Helper lambda for dual output (console + file)
    auto printBoth = [&outFile](const std::string& msg) {
        std::cout << msg;
        if (outFile.is_open()) outFile << msg;
    };

    printBoth("\n");
    printBoth("============================================================\n");
    printBoth("    Neutron Detection Efficiency Analysis (RDataFrame)\n");
    printBoth("============================================================\n");
    printBoth("Directory:  " + std::string(directory) + "\n");
    printBoth("Pattern:    " + std::string(pattern) + "*.root\n");
    printBoth("Threads:    " + std::to_string(nThreads) + "\n");
    printBoth("============================================================\n\n");

    // Enable implicit multi-threading for automatic parallelization
    ROOT::EnableImplicitMT(nThreads);

    //========================================================================//
    // Find all matching ROOT files
    //========================================================================//
    TSystemDirectory dir(directory, directory);
    TList* files = dir.GetListOfFiles();

    if (!files) {
        std::cerr << "Error: Cannot access directory: " << directory << std::endl;
        return;
    }

    // Collect matching files
    std::vector<TString> rootFiles;
    TIter next(files);
    TObject* obj;

    while ((obj = next())) {
        TString name = obj->GetName();
        if (name.EndsWith(".root") && name.Contains(pattern)) {
            TString fullPath = TString(directory) + "/" + name;
            rootFiles.push_back(fullPath);
        }
    }

    if (rootFiles.empty()) {
        std::cerr << "Error: No matching ROOT files found.\n";
        if (outFile.is_open()) outFile.close();
        return;
    }

    printBoth("Found " + std::to_string(rootFiles.size()) + " matching files.\n\n");

    //========================================================================//
    // Group files by energy and track angle/distance
    //========================================================================//
    std::map<Double_t, std::vector<TString>> filesByEnergy;
    std::map<Double_t, Double_t> angleByEnergy;
    std::map<Double_t, Double_t> distanceByEnergy;

    for (const auto& filepath : rootFiles) {
        Double_t energy = parseEnergy(filepath);
        FilenameParams params = parseFilenameParams(filepath);

        if (energy > 0 && params.valid) {
            filesByEnergy[energy].push_back(filepath);
            angleByEnergy[energy] = params.angle;
            distanceByEnergy[energy] = params.distance;
        }
    }

    if (filesByEnergy.empty()) {
        std::cerr << "Error: No files with valid energies found.\n";
        if (outFile.is_open()) outFile.close();
        return;
    }

    //========================================================================//
    // Analyze each energy point
    //========================================================================//
    std::vector<EnergyResults> results;

    printBoth("Analyzing " + std::to_string(filesByEnergy.size()) + " energy points...\n");

    for (const auto& [energy, energyFiles] : filesByEnergy) {
        Double_t angle = angleByEnergy[energy];
        Double_t distance = distanceByEnergy[energy];
        EnergyResults res = analyzeEnergyPoint(energy, angle, distance, energyFiles);
        results.push_back(res);

        std::ostringstream oss;
        oss << "  Energy " << std::setw(6) << std::fixed << std::setprecision(1)
            << energy << " keV @ " << std::setprecision(1) << distance << " cm: "
            << energyFiles.size() << " file(s), " << res.totalEvents << " events, "
            << res.entries << " entries, " << res.captures << " captures\n";
        printBoth(oss.str());
    }

    printBoth("\n");

    // Sort results by energy
    std::sort(results.begin(), results.end(),
              [](const EnergyResults& a, const EnergyResults& b) { return a.energy < b.energy; });

    //========================================================================//
    // Print summary table
    //========================================================================//
    std::ostringstream tableStream;
    tableStream << std::fixed << std::setprecision(1);
    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    tableStream << std::setw(10) << "Energy"
                << std::setw(10) << "Distance"
                << std::setw(12) << "Events"
                << std::setw(12) << "Entries"
                << std::setw(12) << "Captures"
                << std::setw(18) << "Efficiency"
                << std::setw(22) << "Gen Energy"
                << std::setw(22) << "TOF Energy"
                << std::setw(12) << "Mean(T/G)"
                << "\n";
    tableStream << std::setw(10) << "[keV]"
                << std::setw(10) << "[cm]"
                << std::setw(12) << ""
                << std::setw(12) << ""
                << std::setw(12) << ""
                << std::setw(18) << "[%]"
                << std::setw(22) << "[keV]"
                << std::setw(22) << "[keV]"
                << std::setw(12) << ""
                << "\n";
    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------------------\n";

    ULong64_t totalEventsAll = 0;
    ULong64_t totalEntriesAll = 0;
    ULong64_t totalCapturesAll = 0;

    for (const auto& res : results) {
        tableStream << std::setw(10) << res.energy
                    << std::setw(10) << res.distance
                    << std::setw(12) << res.totalEvents
                    << std::setw(12) << res.entries
                    << std::setw(12) << res.captures
                    << std::setw(11) << std::setprecision(3) << 100.0 * res.efficiency
                    << " +/- " << std::setprecision(3) << std::setw(5) << 100.0 * res.efficiencyError
                    << std::setw(10) << std::setprecision(2) << res.meanGenEnergy
                    << " +/- " << std::setprecision(2) << std::setw(6) << res.stderrGenEnergy
                    << std::setw(10) << std::setprecision(2) << res.meanTOFEnergy
                    << " +/- " << std::setprecision(2) << std::setw(6) << res.stderrTOFEnergy
                    << std::setw(12) << std::setprecision(3) << res.meanEnergyRatio
                    << "\n";
        totalEventsAll += res.totalEvents;
        totalEntriesAll += res.entries;
        totalCapturesAll += res.captures;
    }

    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    tableStream << std::setw(10) << "TOTAL"
                << std::setw(10) << ""
                << std::setw(12) << totalEventsAll
                << std::setw(12) << totalEntriesAll
                << std::setw(12) << totalCapturesAll
                << std::setw(11) << std::setprecision(3) << (totalEntriesAll > 0 ? 100.0 * totalCapturesAll / totalEntriesAll : 0.0)
                << "\n";
    tableStream << "============================================================================================================================================================\n\n";

    printBoth(tableStream.str());

    //========================================================================//
    // Create efficiency vs energy plot and fit
    //========================================================================//
    Int_t nPoints = results.size();
    std::vector<Double_t> energies(nPoints), efficiencies(nPoints);
    std::vector<Double_t> energyErrors(nPoints, 0.0), effErrors(nPoints);

    for (Int_t i = 0; i < nPoints; i++) {
        energies[i] = results[i].energy;
        efficiencies[i] = results[i].efficiency;
        effErrors[i] = results[i].efficiencyError;
    }

    TCanvas* c1 = new TCanvas("c1_eff_vs_energy",
        "Detection Efficiency vs Neutron Energy", 800, 600);
    c1->SetGrid();
    c1->SetLeftMargin(0.12);

    TGraphErrors* gEff = new TGraphErrors(nPoints, energies.data(), efficiencies.data(),
                                           energyErrors.data(), effErrors.data());
    gEff->SetTitle("Detection Efficiency vs Neutron Energy;Neutron Energy [keV];Efficiency");
    gEff->SetMarkerStyle(21);
    gEff->SetMarkerSize(0.6);
    gEff->SetMarkerColor(kBlue);
    gEff->SetLineColor(kBlue);
    gEff->Draw("AP");

    //========================================================================//
    // Auto-select best polynomial fit (try pol3 through pol6)
    //========================================================================//
    printBoth("--- Polynomial Fit Selection ---\n\n");

    Int_t bestDegree = -1;
    Double_t bestChi2Ndf = 1e10;
    TF1* bestFit = nullptr;

    Double_t fitRangeLow = energies.front();
    Double_t fitRangeHigh = energies.back();

    for (Int_t deg = 3; deg <= 6; deg++) {
        TString fitName = Form("effFit_pol%d", deg);
        TString fitFormula = Form("pol%d", deg);
        TF1* fitFunc = new TF1(fitName, fitFormula, fitRangeLow, fitRangeHigh);

        gEff->Fit(fitFunc, "RQN");  // R=range, Q=quiet, N=no draw

        Double_t chi2 = fitFunc->GetChisquare();
        Int_t ndf = fitFunc->GetNDF();
        Double_t chi2Ndf = (ndf > 0) ? chi2 / ndf : 1e10;

        std::ostringstream fitMsg;
        fitMsg << "  pol" << deg << ": Chi2/NDF = "
               << std::fixed << std::setprecision(2) << chi2 << " / " << ndf
               << " = " << std::setprecision(4) << chi2Ndf << "\n";
        printBoth(fitMsg.str());

        // Select fit with chi2/ndf closest to 1.0
        if (std::abs(chi2Ndf - 1.0) < std::abs(bestChi2Ndf - 1.0)) {
            bestChi2Ndf = chi2Ndf;
            bestDegree = deg;
            if (bestFit) delete bestFit;
            bestFit = fitFunc;
        } else {
            delete fitFunc;
        }
    }

    std::ostringstream selMsg;
    selMsg << "\n  Selected: pol" << bestDegree
           << " (Chi2/NDF = " << std::fixed << std::setprecision(4) << bestChi2Ndf << ")\n\n";
    printBoth(selMsg.str());

    // Print fit parameters
    printBoth("--- Fit Parameters ---\n\n");
    std::ostringstream paramMsg;
    paramMsg << "  epsilon(E) = p0 + p1*E + p2*E^2 + ... + p" << bestDegree << "*E^" << bestDegree << "\n";
    for (Int_t i = 0; i <= bestDegree; i++) {
        paramMsg << "  p" << i << " = " << std::scientific << std::setprecision(8)
                 << bestFit->GetParameter(i) << " +/- " << bestFit->GetParError(i) << "\n";
    }
    paramMsg << "\n";
    printBoth(paramMsg.str());

    // Draw the selected fit on the plot
    bestFit->SetLineColor(kRed);
    bestFit->SetLineWidth(2);
    bestFit->Draw("SAME");

    TLegend* leg = new TLegend(0.50, 0.75, 0.88, 0.88);
    leg->AddEntry(gEff, "Measured efficiency", "lep");
    leg->AddEntry(bestFit, Form("Polynomial fit (pol%d)", bestDegree), "l");
    leg->SetBorderSize(0);
    leg->Draw();

    c1->Update();
    c1->SaveAs("efficiency_vs_energy.png");

    //========================================================================//
    // Write efficiency fit parameters to file
    //========================================================================//
    std::ofstream fitFile("efficiency_fit.txt");
    if (fitFile.is_open()) {
        fitFile << "# Efficiency polynomial fit parameters\n";
        fitFile << "# epsilon(E) = p0 + p1*E + p2*E^2 + ... + pN*E^N\n";
        fitFile << "# E in keV, epsilon dimensionless (0 to 1)\n";
        fitFile << "# Chi2/NDF = " << std::fixed << std::setprecision(4) << bestChi2Ndf << "\n";
        fitFile << "# Fit range: " << std::setprecision(1) << fitRangeLow
                << " - " << fitRangeHigh << " keV\n";
        fitFile << "DEGREE " << bestDegree << "\n";
        for (Int_t i = 0; i <= bestDegree; i++) {
            fitFile << "p" << i << " " << std::scientific << std::setprecision(12)
                    << bestFit->GetParameter(i) << "\n";
        }
        fitFile.close();
        printBoth("Efficiency fit parameters saved to: efficiency_fit.txt\n");
    } else {
        std::cerr << "Warning: Could not write efficiency_fit.txt\n";
    }

    //========================================================================//
    // Summary
    //========================================================================//
    std::ostringstream plotList;
    plotList << "\nPlots saved:\n"
             << "  efficiency_vs_energy.png (efficiency data + polynomial fit)\n\n";
    printBoth(plotList.str());

    printBoth("============================================================\n");
    printBoth("Efficiency analysis complete!\n");
    printBoth("Results saved to: eff_analysis_results.txt\n");
    printBoth("Fit parameters saved to: efficiency_fit.txt\n");
    printBoth("============================================================\n");

    // Close output file
    if (outFile.is_open()) {
        outFile.close();
    }
}
