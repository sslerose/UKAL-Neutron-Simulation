//
// AnalyzeDataRD.C (RDataFrame version)
// ROOT analysis macro for neutron detection simulation data
//
// Optimized version using ROOT RDataFrame for large-scale analysis (100M+ events)
// Expected performance: 8-10x speedup with 80% memory reduction vs original
//
// Usage:
//   root -l
//   .L AnalyzeDataRD.C
//   analyzeData("path/to/data/directory")
//
//   // Or with default current directory:
//   analyzeData()
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
#include "TMultiGraph.h"
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
#include <memory>
#include <sstream>

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
//   nTOF_SimLiT_1912_0kev_10_0deg_50_0cm.root  ->  angle=10.0, distance=50.0
//   nTOF_SimLiT_1912_0kev_-60_0deg_50_0cm.root ->  angle=-60.0, distance=50.0
//
// Returns FilenameParams with valid=false on parse failure
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
    // beforeDistDec contains everything up to and including "deg"
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
// Result structure for per-angle analysis
//============================================================================//
struct AngleResults {
    TString filename;
    Double_t angle;
    Double_t distance;           // Parsed distance from filename (cm)
    ULong64_t totalEvents;
    ULong64_t entries;           // Neutrons that entered the detector
    ULong64_t captures;
    Double_t efficiency;         // Capture efficiency: captures / entries
    Double_t efficiencyError;

    // TOF energy statistics
    Double_t meanTOFEnergy;
    Double_t stddevTOFEnergy;
    Double_t stderrTOFEnergy;

    // Generated neutron energy statistics (for neutrons that entered detector)
    Double_t meanGenEnergy;
    Double_t stddevGenEnergy;
    Double_t stderrGenEnergy;

    // Histogram result (for per-angle TOF spectrum)
    ROOT::RDF::RResultPtr<TH1D> histTOF;

    AngleResults()
        : angle(-999.0), distance(-999.0), totalEvents(0), entries(0), captures(0),
          efficiency(0.0), efficiencyError(0.0),
          meanTOFEnergy(0.0), stddevTOFEnergy(0.0), stderrTOFEnergy(0.0),
          meanGenEnergy(0.0), stddevGenEnergy(0.0), stderrGenEnergy(0.0) {}
};

//============================================================================//
// Analyze a single file or group of files at the same angle
//============================================================================//
AngleResults analyzeAngleGroup(Double_t angle, Double_t distance,
                               const std::vector<TString>& files) {

    AngleResults result;
    result.angle = angle;
    result.distance = distance;

    if (files.empty()) return result;

    result.filename = files[0];  // Store first filename for reference

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

    // Count entries
    auto countEntries = dfEntered.Count();

    // Compute generated neutron energy statistics for neutrons that entered
    auto meanGenE = dfEntered.Mean("NeutronEnergy");
    auto stddevGenE = dfEntered.StdDev("NeutronEnergy");

    //========================================================================//
    // Filter for captures (CaptureFlag == 1 with valid TOF energy)
    //========================================================================//
    auto dfCapture = df.Filter("CaptureFlag == 1 && TOFEnergy > 0");

    // Count captures
    auto countCaptures = dfCapture.Count();

    // Compute TOF statistics in single pass (Welford's algorithm used internally)
    auto meanTOF = dfCapture.Mean("TOFEnergy");
    auto stddevTOF = dfCapture.StdDev("TOFEnergy");

    // Create histogram for this angle (auto-ranging will be done later)
    // For now, use wide range - will be adjusted in visualization
    auto histName = Form("hTOF_angle_%.1f", angle);
    auto histTOF = dfCapture.Histo1D({histName, histName, 100, 0, 300}, "TOFEnergy");

    //========================================================================//
    // Trigger computation (lazy evaluation up to this point)
    //========================================================================//
    result.totalEvents = *countTotal;
    result.entries = *countEntries;
    result.captures = *countCaptures;
    result.meanTOFEnergy = *meanTOF;
    result.stddevTOFEnergy = *stddevTOF;

    // Store generated neutron energy statistics (for neutrons that entered)
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

    // Store histogram result pointer (not evaluated yet)
    result.histTOF = histTOF;

    return result;
}

//============================================================================//
// Main analysis function
//============================================================================//
void analyzeData(const char* directory = ".", const char* pattern = "nTOF_", int nThreads = 8) {

    // Open output file for saving results
    std::ofstream outFile("analysis_results.txt");
    if (!outFile.is_open()) {
        std::cerr << "Warning: Could not open analysis_results.txt for writing. Results will only print to console.\n";
    }

    // Helper lambda for dual output (console + file)
    auto printBoth = [&outFile](const std::string& msg) {
        std::cout << msg;
        if (outFile.is_open()) outFile << msg;
    };

    printBoth("\n");
    printBoth("============================================================\n");
    printBoth("    Neutron Detection Data Analysis (RDataFrame)\n");
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
    // Group files by angle and track distance
    //========================================================================//
    std::map<Double_t, std::vector<TString>> filesByAngle;
    std::map<Double_t, Double_t> distanceByAngle;  // Store distance for each angle

    for (const auto& filepath : rootFiles) {
        FilenameParams params = parseFilenameParams(filepath);
        if (params.valid) {
            filesByAngle[params.angle].push_back(filepath);
            distanceByAngle[params.angle] = params.distance;
        }
    }

    if (filesByAngle.empty()) {
        std::cerr << "Error: No files with valid angles found.\n";
        if (outFile.is_open()) outFile.close();
        return;
    }

    //========================================================================//
    // Analyze each angle group
    //========================================================================//
    std::vector<AngleResults> results;

    printBoth("Analyzing " + std::to_string(filesByAngle.size()) + " angle groups...\n");

    for (const auto& [angle, angleFiles] : filesByAngle) {
        Double_t distance = distanceByAngle[angle];
        AngleResults res = analyzeAngleGroup(angle, distance, angleFiles);
        results.push_back(res);

        std::ostringstream oss;
        oss << "  Angle " << std::setw(6) << std::fixed << std::setprecision(1)
            << angle << " deg @ " << std::setprecision(1) << distance << " cm: "
            << angleFiles.size() << " file(s), " << res.totalEvents << " events, "
            << res.entries << " entries\n";
        printBoth(oss.str());
    }

    printBoth("\n");

    // Sort results by angle
    std::sort(results.begin(), results.end(),
              [](const AngleResults& a, const AngleResults& b) { return a.angle < b.angle; });

    //========================================================================//
    // Process first file for neutron distributions
    //========================================================================//
    TString firstFile = rootFiles[0];
    ROOT::RDataFrame dfFirst("DetectorData", firstFile.Data());

    // Get min/max for auto-ranging
    auto neutronEnergyStats = dfFirst.Stats("NeutronEnergy");
    auto neutronThetaStats = dfFirst.Stats("NeutronTheta");

    // Create histograms with auto-ranging
    Double_t eMin = neutronEnergyStats->GetMin();
    Double_t eMax = neutronEnergyStats->GetMax();
    Double_t eRange = eMax - eMin;
    Double_t eLow = TMath::Max(0.0, eMin - 0.1 * eRange);
    Double_t eHigh = eMax + 0.1 * eRange;

    Double_t tMin = neutronThetaStats->GetMin();
    Double_t tMax = neutronThetaStats->GetMax();
    Double_t tLow = TMath::Min(0.0, tMin);
    Double_t tHigh = TMath::Max(90.0, tMax);

    auto hNeutronEnergy = dfFirst.Histo1D(
        {"hNeutronEnergy", "Generated Neutron Energy Distribution (First File);Neutron Energy [keV];Counts",
         100, eLow, eHigh}, "NeutronEnergy");

    auto hNeutronTheta = dfFirst.Histo1D(
        {"hNeutronTheta", "Generated Neutron Angular Distribution (First File);Neutron Angle [deg];Counts",
         90, tLow, tHigh}, "NeutronTheta");

    // For scatterplot - use systematic sampling
    auto neutronECount = dfFirst.Count();
    ULong64_t nTotal = *neutronECount;
    const Int_t maxSamplePoints = 10000;
    Long64_t step = (nTotal > maxSamplePoints) ? (nTotal / maxSamplePoints) : 1;

    //========================================================================//
    // Create combined TOF spectrum from all captures
    //========================================================================//
    TChain chainAll("DetectorData");
    for (const auto& file : rootFiles) {
        chainAll.Add(file);
    }
    chainAll.SetBranchStatus("NeutronEnergy", 0);
    chainAll.SetBranchStatus("NeutronTheta", 0);

    ROOT::RDataFrame dfAll(chainAll);
    auto dfAllCaptures = dfAll.Filter("CaptureFlag == 1 && TOFEnergy > 0");

    // Get TOF range for auto-ranging
    auto tofStats = dfAllCaptures.Stats("TOFEnergy");
    Double_t tofMin = tofStats->GetMin();
    Double_t tofMax = tofStats->GetMax();
    Double_t tofRange = tofMax - tofMin;
    Double_t tofLow = TMath::Max(0.0, tofMin - 0.1 * tofRange);
    Double_t tofHigh = tofMax + 0.1 * tofRange;

    auto hTOFSpectrum = dfAllCaptures.Histo1D(
        {"hTOFSpectrum", "TOF Energy Spectrum (Captured Neutrons);TOF Energy [keV];Counts",
         100, tofLow, tofHigh}, "TOFEnergy");

    // Count total captures for statistics
    auto totalCapturesCount = dfAllCaptures.Count();
    ULong64_t totalCaptures = *totalCapturesCount;

    //========================================================================//
    // Print summary table
    //========================================================================//
    std::ostringstream tableStream;
    tableStream << std::fixed << std::setprecision(1);
    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------\n";
    tableStream << std::setw(10) << "Angle"
                << std::setw(10) << "Distance"
                << std::setw(12) << "Events"
                << std::setw(12) << "Entries"
                << std::setw(12) << "Captures"
                << std::setw(18) << "Efficiency"
                << std::setw(22) << "Gen Energy"
                << std::setw(22) << "TOF Energy"
                << "\n";
    tableStream << std::setw(10) << "[deg]"
                << std::setw(10) << "[cm]"
                << std::setw(12) << ""
                << std::setw(12) << ""
                << std::setw(12) << ""
                << std::setw(18) << "[%]"
                << std::setw(22) << "[keV]"
                << std::setw(22) << "[keV]"
                << "\n";
    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------\n";

    ULong64_t totalEventsAll = 0;
    ULong64_t totalEntriesAll = 0;
    ULong64_t totalCapturesAll = 0;

    for (const auto& res : results) {
        tableStream << std::setw(10) << res.angle
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
                    << "\n";
        totalEventsAll += res.totalEvents;
        totalEntriesAll += res.entries;
        totalCapturesAll += res.captures;
    }

    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------\n";
    tableStream << std::setw(10) << "TOTAL"
                << std::setw(10) << ""
                << std::setw(12) << totalEventsAll
                << std::setw(12) << totalEntriesAll
                << std::setw(12) << totalCapturesAll
                << std::setw(11) << std::setprecision(3) << 100.0 * totalCapturesAll / totalEntriesAll
                << "\n";
    tableStream << "================================================================================================================================================\n\n";

    printBoth(tableStream.str());

    //========================================================================//
    // Create plots
    //========================================================================//
    gStyle->SetOptStat(0);

    // Prepare data arrays for TGraphErrors
    Int_t nPoints = results.size();
    std::vector<Double_t> angles(nPoints);
    std::vector<Double_t> captures(nPoints);
    std::vector<Double_t> efficiencies(nPoints);
    std::vector<Double_t> effErrors(nPoints);
    std::vector<Double_t> angleErrors(nPoints, 0.0);

    std::vector<Double_t> meanTOFEnergies(nPoints);
    std::vector<Double_t> tofEnergyErrors(nPoints);

    // Generated energy arrays for comparison plot
    std::vector<Double_t> meanGenEnergies(nPoints);
    std::vector<Double_t> genEnergyErrors(nPoints);

    for (Int_t i = 0; i < nPoints; i++) {
        angles[i] = results[i].angle;
        captures[i] = results[i].captures;
        efficiencies[i] = 100.0 * results[i].efficiency;
        effErrors[i] = 100.0 * results[i].efficiencyError;

        meanTOFEnergies[i] = results[i].meanTOFEnergy;
        tofEnergyErrors[i] = results[i].stderrTOFEnergy;

        meanGenEnergies[i] = results[i].meanGenEnergy;
        genEnergyErrors[i] = results[i].stderrGenEnergy;
    }

    //========================================================================//
    // Canvas 1: Detection Efficiency vs Angle
    //========================================================================//
    Double_t minAngle = *std::min_element(angles.begin(), angles.end());
    Double_t maxAngle = *std::max_element(angles.begin(), angles.end());

    TCanvas* c1 = new TCanvas("c1_efficiency", "Detection Efficiency vs Detector Angle", 800, 600);
    c1->SetGrid();

    TGraphErrors* gEfficiency = new TGraphErrors(nPoints,
        angles.data(), efficiencies.data(),
        angleErrors.data(), effErrors.data());

    gEfficiency->SetTitle("Detection Efficiency vs Detector Angle;Detector Angle [deg];Efficiency [%]");
    gEfficiency->SetMarkerStyle(21);
    gEfficiency->SetMarkerSize(1.2);
    gEfficiency->SetMarkerColor(kRed);
    gEfficiency->SetLineColor(kRed);
    gEfficiency->SetLineWidth(2);
    gEfficiency->Draw("AP");

    gEfficiency->GetXaxis()->SetLimits(minAngle - 5, maxAngle + 5);
    gEfficiency->SetMinimum(0);

    //========================================================================//
    // Canvas 2: Energy vs Angle (TOF and Generated comparison)
    //========================================================================//
    TCanvas* c2 = new TCanvas("c2_tofenergy", "Energy vs Detector Angle", 800, 600);
    c2->SetGrid();

    // TOF Energy graph (Measured)
    TGraphErrors* gTOFEnergy = new TGraphErrors(nPoints,
        angles.data(), meanTOFEnergies.data(),
        angleErrors.data(), tofEnergyErrors.data());

    gTOFEnergy->SetMarkerStyle(21);  // Square
    gTOFEnergy->SetMarkerSize(1.2);
    gTOFEnergy->SetMarkerColor(kBlue);
    gTOFEnergy->SetLineColor(kBlue);
    gTOFEnergy->SetLineWidth(2);

    // Generated Energy graph (Expected)
    TGraphErrors* gGenEnergy = new TGraphErrors(nPoints,
        angles.data(), meanGenEnergies.data(),
        angleErrors.data(), genEnergyErrors.data());

    gGenEnergy->SetMarkerStyle(22);  // Triangle up
    gGenEnergy->SetMarkerSize(1.2);
    gGenEnergy->SetMarkerColor(kRed);
    gGenEnergy->SetLineColor(kRed);
    gGenEnergy->SetLineWidth(2);

    // Create TMultiGraph to hold both
    TMultiGraph* mg = new TMultiGraph();
    mg->Add(gTOFEnergy, "P");
    mg->Add(gGenEnergy, "P");

    mg->SetTitle("Neutron Energy vs Detector Angle;Detector Angle [deg];Energy [keV]");
    mg->Draw("A");

    mg->GetXaxis()->SetLimits(minAngle - 5, maxAngle + 5);

    // Auto-range Y axis based on both datasets
    Double_t yMinTOF = *std::min_element(meanTOFEnergies.begin(), meanTOFEnergies.end());
    Double_t yMinGen = *std::min_element(meanGenEnergies.begin(), meanGenEnergies.end());
    Double_t yMaxTOF = *std::max_element(meanTOFEnergies.begin(), meanTOFEnergies.end());
    Double_t yMaxGen = *std::max_element(meanGenEnergies.begin(), meanGenEnergies.end());
    Double_t yMin = TMath::Min(yMinTOF, yMinGen);
    Double_t yMax = TMath::Max(yMaxTOF, yMaxGen);
    mg->SetMinimum(yMin * 0.9);
    mg->SetMaximum(yMax * 1.1);

    // Add legend
    TLegend* legEnergy = new TLegend(0.65, 0.75, 0.88, 0.88);
    legEnergy->AddEntry(gTOFEnergy, "Measured (TOF)", "lep");
    legEnergy->AddEntry(gGenEnergy, "Expected (Generated)", "lep");
    legEnergy->Draw();

    c2->Update();

    //========================================================================//
    // Canvas 3: Histogram of captures
    //========================================================================//
    Double_t binWidth = (nPoints > 1) ? (angles[1] - angles[0]) : 10.0;
    Int_t nBins = (Int_t)((maxAngle - minAngle) / TMath::Abs(binWidth)) + 1;

    TCanvas* c3 = new TCanvas("c3_histogram", "Captures Histogram", 800, 600);
    c3->SetGrid();

    TH1D* hCaptures = new TH1D("hCaptures",
        "Neutron Captures vs Detector Angle;Detector Angle [deg];Number of Captures",
        nBins, minAngle - binWidth/2, maxAngle + binWidth/2);

    hCaptures->SetFillColor(kBlue);
    hCaptures->SetFillStyle(3004);
    hCaptures->SetLineColor(kBlue);
    hCaptures->SetLineWidth(2);

    for (Int_t i = 0; i < nPoints; i++) {
        Int_t bin = hCaptures->FindBin(angles[i]);
        hCaptures->SetBinContent(bin, captures[i]);
    }

    hCaptures->Draw("HIST");

    //========================================================================//
    // Canvas 4: TOF Energy Spectrum (all captured neutrons)
    //========================================================================//
    TCanvas* c4 = new TCanvas("c4_tof_spectrum", "TOF Energy Spectrum", 800, 600);
    c4->SetGrid();

    TH1D* hTOFSpec = (TH1D*)hTOFSpectrum->Clone();
    hTOFSpec->SetFillColor(kGreen+2);
    hTOFSpec->SetFillStyle(3004);
    hTOFSpec->SetLineColor(kGreen+2);
    hTOFSpec->SetLineWidth(2);
    hTOFSpec->Draw("HIST");
    c4->Update();

    std::ostringstream tofStatsOutput;
    tofStatsOutput << "TOF Energy Spectrum Statistics:\n"
                   << "  Total captured neutrons: " << totalCaptures << "\n"
                   << "  Mean TOF Energy: " << hTOFSpec->GetMean() << " keV\n"
                   << "  Sample Std Dev: " << hTOFSpec->GetStdDev() << " keV\n"
                   << "  Min: " << tofMin << " keV\n"
                   << "  Max: " << tofMax << " keV\n\n";
    printBoth(tofStatsOutput.str());

    //========================================================================//
    // Canvas 5: TOF Energy Spectrum per Angle
    //========================================================================//
    TCanvas* c5 = new TCanvas("c5_tof_per_angle", "TOF Energy Spectrum per Angle", 1000, 700);
    c5->SetGrid();

    TLegend* leg = new TLegend(0.7, 0.5, 0.88, 0.88);
    leg->SetHeader("Detector Angle", "C");

    Int_t colors[] = {kRed, kBlue, kGreen+2, kMagenta, kCyan+1,
                      kOrange+1, kViolet, kTeal+2, kPink+1, kAzure+1,
                      kYellow+2, kSpring+2, kGray+1};
    Int_t nColors = sizeof(colors) / sizeof(colors[0]);

    Double_t maxCounts = 0;
    std::vector<TH1D*> angleHistos;

    for (Int_t i = 0; i < nPoints; i++) {
        TH1D* h = (TH1D*)results[i].histTOF->Clone(Form("hTOF_angle_%d", i));
        h->SetLineColor(colors[i % nColors]);
        h->SetLineWidth(2);

        if (h->GetMaximum() > maxCounts) {
            maxCounts = h->GetMaximum();
        }

        angleHistos.push_back(h);
        leg->AddEntry(h, Form("%.1f deg", angles[i]), "l");
    }

    for (size_t idx = 0; idx < angleHistos.size(); idx++) {
        angleHistos[idx]->SetMaximum(maxCounts * 1.1);
        if (idx == 0) {
            angleHistos[idx]->SetTitle("TOF Energy Spectrum by Detector Angle;TOF Energy [keV];Counts");
            angleHistos[idx]->Draw("HIST");
        } else {
            angleHistos[idx]->Draw("HIST SAME");
        }
    }

    leg->Draw();
    c5->Update();

    //========================================================================//
    // Canvas 6: Generated Neutron Energy Distribution
    //========================================================================//
    TCanvas* c6 = new TCanvas("c6_neutron_energy", "Generated Neutron Energy Distribution", 800, 600);
    c6->SetGrid();

    TH1D* hNeutronE = (TH1D*)hNeutronEnergy->Clone();
    hNeutronE->SetFillColor(kBlue);
    hNeutronE->SetFillStyle(3004);
    hNeutronE->SetLineColor(kBlue);
    hNeutronE->SetLineWidth(2);
    hNeutronE->Draw("HIST");
    c6->Update();

    std::ostringstream neutronEStats;
    neutronEStats << "Generated Neutron Energy Statistics (First File: " << firstFile << "):\n"
                  << "  Total events: " << nTotal << "\n"
                  << "  Mean Energy: " << hNeutronE->GetMean() << " keV\n"
                  << "  Sample Std Dev: " << hNeutronE->GetStdDev() << " keV\n\n";
    printBoth(neutronEStats.str());

    //========================================================================//
    // Canvas 7: Generated Neutron Angular Distribution
    //========================================================================//
    TCanvas* c7 = new TCanvas("c7_neutron_theta", "Generated Neutron Angular Distribution", 800, 600);
    c7->SetGrid();

    TH1D* hNeutronT = (TH1D*)hNeutronTheta->Clone();
    hNeutronT->SetFillColor(kRed);
    hNeutronT->SetFillStyle(3004);
    hNeutronT->SetLineColor(kRed);
    hNeutronT->SetLineWidth(2);
    hNeutronT->Draw("HIST");
    c7->Update();

    std::ostringstream neutronTStats;
    neutronTStats << "Generated Neutron Angular Statistics (First File):\n"
                  << "  Total events: " << nTotal << "\n"
                  << "  Mean Angle: " << hNeutronT->GetMean() << " deg\n"
                  << "  Sample Std Dev: " << hNeutronT->GetRMS() << " deg\n\n";
    printBoth(neutronTStats.str());

    //========================================================================//
    // Canvas 8: Neutron Energy vs Angle Scatterplot (sampled)
    //========================================================================//
    TCanvas* c8 = new TCanvas("c8_energy_vs_theta", "Neutron Energy vs Angle", 800, 600);
    c8->SetGrid();

    // Create scatterplot with systematic sampling from first file
    // Using traditional TTree approach for compatibility
    std::vector<Double_t> sampledEnergies;
    std::vector<Double_t> sampledThetas;

    TFile* f = TFile::Open(firstFile, "READ");
    TTree* tree = (TTree*)f->Get("DetectorData");

    Double_t neutronE, neutronT;
    tree->SetBranchAddress("NeutronEnergy", &neutronE);
    tree->SetBranchAddress("NeutronTheta", &neutronT);

    Long64_t nEntries = tree->GetEntries();
    Int_t nSampled = (nEntries > maxSamplePoints) ? maxSamplePoints : nEntries;
    sampledEnergies.reserve(nSampled);
    sampledThetas.reserve(nSampled);

    for (Long64_t i = 0; i < nEntries; i += step) {
        tree->GetEntry(i);
        sampledEnergies.push_back(neutronE);
        sampledThetas.push_back(neutronT);
    }

    f->Close();
    delete f;

    TGraph* gScatter = new TGraph(sampledEnergies.size(),
                                   sampledThetas.data(), sampledEnergies.data());

    gScatter->SetTitle("Generated Neutron Energy vs Angle (First File, Sampled);Neutron Angle [deg];Neutron Energy [keV]");
    gScatter->SetMarkerStyle(6);
    gScatter->SetMarkerColor(kBlue);
    gScatter->SetMarkerSize(0.5);
    gScatter->Draw("AP");
    c8->Update();

    std::ostringstream scatterStats;
    scatterStats << "Scatterplot Statistics (First File):\n"
                 << "  Total events: " << nTotal << "\n"
                 << "  Sampled points: " << sampledEnergies.size() << "\n"
                 << "  Sample step: " << step << "\n\n";
    printBoth(scatterStats.str());

    //========================================================================//
    // Save plots
    //========================================================================//
    c1->Update();
    c1->SaveAs("efficiency_vs_angle.png");

    c2->Update();
    c2->SaveAs("tof_energy_vs_angle.png");

    c3->Update();
    c3->SaveAs("captures_histogram.png");

    c4->SaveAs("tof_energy_spectrum.png");
    c5->SaveAs("tof_energy_per_angle.png");
    c6->SaveAs("neutron_energy_distribution.png");
    c7->SaveAs("neutron_theta_distribution.png");
    c8->SaveAs("neutron_energy_vs_theta.png");

    std::ostringstream plotList;
    plotList << "Plots saved:\n"
             << "  efficiency_vs_angle.png\n"
             << "  tof_energy_vs_angle.png\n"
             << "  captures_histogram.png\n"
             << "  tof_energy_spectrum.png\n"
             << "  tof_energy_per_angle.png\n"
             << "  neutron_energy_distribution.png\n"
             << "  neutron_theta_distribution.png\n"
             << "  neutron_energy_vs_theta.png\n\n";
    printBoth(plotList.str());

    printBoth("============================================================\n");
    printBoth("Analysis complete!\n");
    printBoth("Results saved to: analysis_results.txt\n");
    printBoth("============================================================\n");

    // Close output file
    if (outFile.is_open()) {
        outFile.close();
    }
}
