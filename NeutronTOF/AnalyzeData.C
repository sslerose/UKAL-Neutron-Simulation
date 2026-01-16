//
// analyzeData.C
// ROOT analysis macro for neutron detection simulation data
//
// Analyzes multiple ROOT files from angle scan simulations.
// Extracts detector angle from filename, counts capture events,
// and analyzes TOF energy distributions per angle.
//
// Filename format expected: nDet_SimLiT_<beamE>keV_<angle>deg.root
//   Example: nDet_SimLiT_1912_0keV_30_0deg.root -> angle = 30.0 deg
//            nDet_SimLiT_1912_0keV_-60_0deg.root -> angle = -60.0 deg
//
// Usage:
//   root -l
//   .L analyzeData.C
//   analyzeData("path/to/data/directory")
//
//   // Or with default current directory:
//   analyzeData()
//

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TString.h"
#include "TAxis.h"
#include "TMath.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>

//============================================================================//
// Parse detector angle from filename
// Expected formats:
//   nDet_SimLiT_1912_0keV_30_0deg.root  ->  30.0
//   nDet_SimLiT_1912_0keV_-60_0deg.root -> -60.0
//   nDet_Gun_50keV_45_5deg.root         ->  45.5
//
// Returns -999.0 on parse failure (since negative angles are valid)
//============================================================================//
Double_t parseAngleFromFilename(const TString& filename) {
    
    const Double_t INVALID_ANGLE = -999.0;
    
    // Find "deg" and work backwards to find the angle
    Int_t degPos = filename.Index("deg");
    if (degPos == -1) {
        std::cerr << "Warning: 'deg' not found in filename: " << filename << std::endl;
        return INVALID_ANGLE;
    }
    
    // Extract everything before "deg"
    TString beforeDeg = filename(0, degPos);
    
    // Find last underscore (separates decimal part, e.g., "_0" in "30_0deg" or "-60_0deg")
    Int_t lastUnderscore = beforeDeg.Last('_');
    if (lastUnderscore == -1) {
        std::cerr << "Warning: Cannot parse angle from: " << filename << std::endl;
        return INVALID_ANGLE;
    }
    
    // Extract decimal part (after last underscore, before "deg")
    TString decPart = beforeDeg(lastUnderscore + 1, beforeDeg.Length() - lastUnderscore - 1);
    
    // Find second-to-last underscore (before integer part, including possible negative sign)
    TString beforeLastUnderscore = beforeDeg(0, lastUnderscore);
    Int_t secondLastUnderscore = beforeLastUnderscore.Last('_');
    if (secondLastUnderscore == -1) {
        std::cerr << "Warning: Cannot parse angle from: " << filename << std::endl;
        return INVALID_ANGLE;
    }
    
    // Extract integer part (may include negative sign, e.g., "-60" or "30")
    TString intPart = beforeDeg(secondLastUnderscore + 1, lastUnderscore - secondLastUnderscore - 1);
    
    // Combine: "-60" + "." + "0" = "-60.0" or "30" + "." + "0" = "30.0"
    TString angleStr = intPart + "." + decPart;
    Double_t angle = angleStr.Atof();
    
    // Debug output (comment out for production)
    // std::cout << "DEBUG: " << filename << " -> intPart=" << intPart 
    //           << ", decPart=" << decPart << ", angle=" << angle << std::endl;
    
    return angle;
}

//============================================================================//
// Count captures and collect TOF energy in a single ROOT file
//============================================================================//
struct FileResults {
    TString filename;
    Double_t angle;
    Long64_t totalEvents;
    Long64_t captures;
    Double_t efficiency;
    Double_t efficiencyError;
    
    // TOF energy statistics
    Double_t meanTOFEnergy;      // Mean TOF energy [keV]
    Double_t stddevTOFEnergy;    // Standard deviation [keV]
    Double_t stderrTOFEnergy;    // Standard error of mean [keV]
};

FileResults analyzeFile(const TString& filepath) {
    
    const Double_t INVALID_ANGLE = -999.0;
    
    FileResults result;
    result.filename = filepath;
    result.angle = INVALID_ANGLE;
    result.totalEvents = 0;
    result.captures = 0;
    result.efficiency = 0.0;
    result.efficiencyError = 0.0;
    result.meanTOFEnergy = 0.0;
    result.stddevTOFEnergy = 0.0;
    result.stderrTOFEnergy = 0.0;
    
    // Parse angle from filename
    result.angle = parseAngleFromFilename(filepath);
    if (result.angle <= INVALID_ANGLE + 1.0) {  // Check for invalid (allows negative angles)
        return result;
    }
    
    // Open file
    TFile* file = TFile::Open(filepath, "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file: " << filepath << std::endl;
        return result;
    }
    
    // Get tree
    TTree* tree = (TTree*)file->Get("DetectorData");
    if (!tree) {
        std::cerr << "Error: Cannot find DetectorData tree in: " << filepath << std::endl;
        file->Close();
        return result;
    }
    
    // Set up branches
    Int_t captureFlag;
    Double_t tofEnergy;
    tree->SetBranchAddress("CaptureFlag", &captureFlag);
    tree->SetBranchAddress("TOFEnergy", &tofEnergy);
    
    // Count events and captures, collect TOF energies
    result.totalEvents = tree->GetEntries();
    std::vector<Double_t> tofEnergies;
    
    for (Long64_t i = 0; i < result.totalEvents; i++) {
        tree->GetEntry(i);
        if (captureFlag == 1) {
            result.captures++;
            if (tofEnergy > 0) {  // Valid TOF energy
                tofEnergies.push_back(tofEnergy);
            }
        }
    }
    
    // Calculate efficiency with binomial error
    if (result.totalEvents > 0) {
        result.efficiency = (Double_t)result.captures / result.totalEvents;
        // Binomial error: sqrt(p*(1-p)/N)
        result.efficiencyError = TMath::Sqrt(result.efficiency * (1.0 - result.efficiency) / result.totalEvents);
    }
    
    // Calculate TOF energy statistics
    if (!tofEnergies.empty()) {
        // Mean
        Double_t sum = 0.0;
        for (const auto& e : tofEnergies) {
            sum += e;
        }
        result.meanTOFEnergy = sum / tofEnergies.size();
        
        // Standard deviation
        Double_t sumSq = 0.0;
        for (const auto& e : tofEnergies) {
            sumSq += (e - result.meanTOFEnergy) * (e - result.meanTOFEnergy);
        }
        if (tofEnergies.size() > 1) {
            result.stddevTOFEnergy = TMath::Sqrt(sumSq / (tofEnergies.size() - 1));
            result.stderrTOFEnergy = result.stddevTOFEnergy / TMath::Sqrt(tofEnergies.size());
        }
    }
    
    file->Close();
    return result;
}

//============================================================================//
// Main analysis function
//============================================================================//
void analyzeData(const char* directory = ".", const char* pattern = "nDet_") {
    
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "           Neutron Detection Data Analysis\n";
    std::cout << "============================================================\n";
    std::cout << "Directory: " << directory << "\n";
    std::cout << "Pattern:   " << pattern << "*.root\n";
    std::cout << "============================================================\n\n";
    
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
        return;
    }
    
    std::cout << "Found " << rootFiles.size() << " matching files.\n\n";
    
    //========================================================================//
    // Analyze each file
    //========================================================================//
    std::vector<FileResults> results;
    const Double_t INVALID_ANGLE = -999.0;
    
    for (const auto& filepath : rootFiles) {
        FileResults res = analyzeFile(filepath);
        if (res.angle > INVALID_ANGLE + 1.0) {  // Valid angle (allows negatives)
            results.push_back(res);
        }
    }
    
    // Sort by angle
    std::sort(results.begin(), results.end(), 
              [](const FileResults& a, const FileResults& b) { return a.angle < b.angle; });
    
    //========================================================================//
    // Print summary table
    //========================================================================//
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "------------------------------------------------------------------------------------------------------\n";
    std::cout << std::setw(10) << "Angle" 
              << std::setw(12) << "Events"
              << std::setw(12) << "Captures"
              << std::setw(18) << "Efficiency"
              << std::setw(22) << "TOF Energy"
              << "\n";
    std::cout << std::setw(10) << "[deg]" 
              << std::setw(12) << ""
              << std::setw(12) << ""
              << std::setw(18) << "[%]"
              << std::setw(22) << "[keV]"
              << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------\n";
    
    Long64_t totalEventsAll = 0;
    Long64_t totalCapturesAll = 0;
    
    for (const auto& res : results) {
        std::cout << std::setw(10) << res.angle
                  << std::setw(12) << res.totalEvents
                  << std::setw(12) << res.captures
                  << std::setw(11) << std::setprecision(3) << 100.0 * res.efficiency
                  << " +/- " << std::setprecision(3) << std::setw(5) << 100.0 * res.efficiencyError
                  << std::setw(10) << std::setprecision(2) << res.meanTOFEnergy
                  << " +/- " << std::setprecision(2) << std::setw(6) << res.stderrTOFEnergy
                  << "\n";
        totalEventsAll += res.totalEvents;
        totalCapturesAll += res.captures;
    }
    
    std::cout << "------------------------------------------------------------------------------------------------------\n";
    std::cout << std::setw(10) << "TOTAL"
              << std::setw(12) << totalEventsAll
              << std::setw(12) << totalCapturesAll
              << std::setw(11) << std::setprecision(3) << 100.0 * totalCapturesAll / totalEventsAll
              << "\n";
    std::cout << "======================================================================================================\n\n";
    
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
    std::vector<Double_t> angleErrors(nPoints, 0.0);  // No x-error
    std::vector<Double_t> captureErrors(nPoints);
    
    // TOF energy data
    std::vector<Double_t> meanTOFEnergies(nPoints);
    std::vector<Double_t> tofEnergyErrors(nPoints);
    
    for (Int_t i = 0; i < nPoints; i++) {
        angles[i] = results[i].angle;
        captures[i] = results[i].captures;
        efficiencies[i] = 100.0 * results[i].efficiency;  // Convert to %
        effErrors[i] = 100.0 * results[i].efficiencyError;
        captureErrors[i] = TMath::Sqrt(results[i].captures);  // Poisson error
        
        meanTOFEnergies[i] = results[i].meanTOFEnergy;
        tofEnergyErrors[i] = results[i].stderrTOFEnergy;
    }
    
    //========================================================================//
    // Canvas 1: Captures vs Angle (no error bars)
    //========================================================================//
    TCanvas* c1 = new TCanvas("c1_captures", "Captures vs Detector Angle", 800, 600);
    c1->SetGrid();
    
    TGraph* gCaptures = new TGraph(nPoints, angles.data(), captures.data());
    
    gCaptures->SetTitle("Neutron Captures vs Detector Angle;Detector Angle [deg];Number of Captures");
    gCaptures->SetMarkerStyle(21);
    gCaptures->SetMarkerSize(1.2);
    gCaptures->SetMarkerColor(kBlue);
    gCaptures->SetLineColor(kBlue);
    gCaptures->SetLineWidth(2);
    gCaptures->Draw("AP");
    
    // Adjust axis ranges (handle negative angles)
    Double_t minAngle = *std::min_element(angles.begin(), angles.end());
    Double_t maxAngle = *std::max_element(angles.begin(), angles.end());
    gCaptures->GetXaxis()->SetLimits(minAngle - 5, maxAngle + 5);
    gCaptures->SetMinimum(0);
    
    //========================================================================//
    // Canvas 2: Detection Efficiency vs Angle
    //========================================================================//
    TCanvas* c2 = new TCanvas("c2_efficiency", "Detection Efficiency vs Detector Angle", 800, 600);
    c2->SetGrid();
    
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
    // Canvas 3: TOF Energy vs Angle
    //========================================================================//
    TCanvas* c3 = new TCanvas("c3_tofenergy", "TOF Energy vs Detector Angle", 800, 600);
    c3->SetGrid();
    
    TGraphErrors* gTOFEnergy = new TGraphErrors(nPoints,
        angles.data(), meanTOFEnergies.data(),
        angleErrors.data(), tofEnergyErrors.data());
    
    gTOFEnergy->SetTitle("Mean TOF Energy vs Detector Angle;Detector Angle [deg];TOF Energy [keV]");
    gTOFEnergy->SetMarkerStyle(21);
    gTOFEnergy->SetMarkerSize(1.2);
    gTOFEnergy->SetMarkerColor(kGreen+2);
    gTOFEnergy->SetLineColor(kGreen+2);
    gTOFEnergy->SetLineWidth(2);
    gTOFEnergy->Draw("AP");
    
    gTOFEnergy->GetXaxis()->SetLimits(minAngle - 5, maxAngle + 5);
    gTOFEnergy->SetMinimum(0);
    
    //========================================================================//
    // Canvas 4: Histogram version of captures (no error bars)
    //========================================================================//
    // Use min/max already calculated above
    Double_t binWidth = (nPoints > 1) ? (angles[1] - angles[0]) : 10.0;
    Int_t nBins = (Int_t)((maxAngle - minAngle) / TMath::Abs(binWidth)) + 1;
    
    TCanvas* c4 = new TCanvas("c4_histogram", "Captures Histogram", 800, 600);
    c4->SetGrid();
    
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
        // No error bars - removed SetBinError
    }
    
    hCaptures->Draw("HIST");  // HIST option draws without error bars
    
    //========================================================================//
    // Save plots
    //========================================================================//
    c1->SaveAs("captures_vs_angle.png");
    c2->SaveAs("efficiency_vs_angle.png");
    c3->SaveAs("tof_energy_vs_angle.png");
    c4->SaveAs("captures_histogram.png");
    
    std::cout << "Plots saved:\n";
    std::cout << "  captures_vs_angle.png\n";
    std::cout << "  efficiency_vs_angle.png\n";
    std::cout << "  tof_energy_vs_angle.png\n";
    std::cout << "  captures_histogram.png\n";
    std::cout << "\n";
}

//============================================================================//
// Alternative: analyze a specific list of files
//============================================================================//
void analyzeFiles(std::vector<TString> filenames) {
    
    const Double_t INVALID_ANGLE = -999.0;
    std::vector<FileResults> results;
    
    for (const auto& filepath : filenames) {
        FileResults res = analyzeFile(filepath);
        if (res.angle > INVALID_ANGLE + 1.0) {
            results.push_back(res);
            std::cout << "  " << filepath << " -> " << res.angle << " deg, "
                      << res.captures << " captures\n";
        }
    }
    
    // Sort and continue with plotting as in main function...
}