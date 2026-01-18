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
    
    // Store individual TOF energies for histogramming
    std::vector<Double_t> tofEnergyValues;
    
    // Store generated neutron parameters (all events)
    std::vector<Double_t> neutronEnergyValues;
    std::vector<Double_t> neutronThetaValues;
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
    Double_t neutronEnergy;
    Double_t neutronTheta;
    tree->SetBranchAddress("CaptureFlag", &captureFlag);
    tree->SetBranchAddress("TOFEnergy", &tofEnergy);
    tree->SetBranchAddress("NeutronEnergy", &neutronEnergy);
    tree->SetBranchAddress("NeutronTheta", &neutronTheta);
    
    // Count events and captures, collect TOF energies and neutron parameters
    result.totalEvents = tree->GetEntries();
    std::vector<Double_t> tofEnergies;
    
    for (Long64_t i = 0; i < result.totalEvents; i++) {
        tree->GetEntry(i);
        
        // Store generated neutron parameters (all events)
        result.neutronEnergyValues.push_back(neutronEnergy);
        result.neutronThetaValues.push_back(neutronTheta);
        
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
        // Store values for later histogramming
        result.tofEnergyValues = tofEnergies;
        
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
    
    // Memory optimization: clear neutron data for all files except first
    // (distributions are identical, only first file's data is used for histograms)
    for (size_t i = 1; i < results.size(); i++) {
        results[i].neutronEnergyValues.clear();
        results[i].neutronEnergyValues.shrink_to_fit();
        results[i].neutronThetaValues.clear();
        results[i].neutronThetaValues.shrink_to_fit();
    }
    
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
    
    // TOF energy data
    std::vector<Double_t> meanTOFEnergies(nPoints);
    std::vector<Double_t> tofEnergyErrors(nPoints);
    
    for (Int_t i = 0; i < nPoints; i++) {
        angles[i] = results[i].angle;
        captures[i] = results[i].captures;
        efficiencies[i] = 100.0 * results[i].efficiency;  // Convert to %
        effErrors[i] = 100.0 * results[i].efficiencyError;
        
        meanTOFEnergies[i] = results[i].meanTOFEnergy;
        tofEnergyErrors[i] = results[i].stderrTOFEnergy;
    }
    
    //========================================================================//
    // Canvas 1: Detection Efficiency vs Angle
    //========================================================================//
    // Calculate min/max angles for axis limits
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
    // Canvas 2: TOF Energy vs Angle
    //========================================================================//
    TCanvas* c2 = new TCanvas("c2_tofenergy", "TOF Energy vs Detector Angle", 800, 600);
    c2->SetGrid();
    
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
    // Canvas 3: Histogram version of captures (no error bars)
    //========================================================================//
    // Use min/max already calculated above
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
        // No error bars - removed SetBinError
    }
    
    hCaptures->Draw("HIST");  // HIST option draws without error bars
    
    //========================================================================//
    // Canvas 4: TOF Energy Spectrum (all captured neutrons)
    //========================================================================//
    // Collect all TOF energies from all angles
    std::vector<Double_t> allTOFEnergies;
    for (const auto& res : results) {
        for (const auto& e : res.tofEnergyValues) {
            allTOFEnergies.push_back(e);
        }
    }
    
    TCanvas* c4 = new TCanvas("c4_tof_spectrum", "TOF Energy Spectrum", 800, 600);
    c4->SetGrid();
    
    if (!allTOFEnergies.empty()) {
        // Determine histogram range from data
        Double_t minE = *std::min_element(allTOFEnergies.begin(), allTOFEnergies.end());
        Double_t maxE = *std::max_element(allTOFEnergies.begin(), allTOFEnergies.end());
        
        // Add some padding and round to nice values
        Double_t range = maxE - minE;
        Double_t lowEdge = TMath::Max(0.0, minE - 0.1 * range);
        Double_t highEdge = maxE + 0.1 * range;
        
        TH1D* hTOFSpectrum = new TH1D("hTOFSpectrum",
            "TOF Energy Spectrum (Captured Neutrons);TOF Energy [keV];Counts",
            100, lowEdge, highEdge);
        
        hTOFSpectrum->SetFillColor(kGreen+2);
        hTOFSpectrum->SetFillStyle(3004);
        hTOFSpectrum->SetLineColor(kGreen+2);
        hTOFSpectrum->SetLineWidth(2);
        
        // Fill histogram with all TOF energies
        for (const auto& e : allTOFEnergies) {
            hTOFSpectrum->Fill(e);
        }
        
        hTOFSpectrum->Draw("HIST");
        
        // Add statistics box info
        std::cout << "TOF Energy Spectrum Statistics:\n";
        std::cout << "  Total captured neutrons: " << allTOFEnergies.size() << "\n";
        std::cout << "  Mean TOF Energy: " << hTOFSpectrum->GetMean() << " keV\n";
        std::cout << "  RMS: " << hTOFSpectrum->GetRMS() << " keV\n";
        std::cout << "  Min: " << minE << " keV\n";
        std::cout << "  Max: " << maxE << " keV\n\n";
    }
    else {
        std::cout << "Warning: No TOF energy data to histogram.\n\n";
    }
    
    //========================================================================//
    // Canvas 5: TOF Energy Spectrum per Angle (stacked or overlaid)
    //========================================================================//
    TCanvas* c5 = new TCanvas("c5_tof_per_angle", "TOF Energy Spectrum per Angle", 1000, 700);
    c5->SetGrid();
    
    // Create legend
    TLegend* leg = new TLegend(0.7, 0.5, 0.88, 0.88);
    leg->SetHeader("Detector Angle", "C");
    
    // Color palette for different angles
    Int_t colors[] = {kRed, kBlue, kGreen+2, kMagenta, kCyan+1, 
                      kOrange+1, kViolet, kTeal+2, kPink+1, kAzure+1,
                      kYellow+2, kSpring+2, kGray+1};
    Int_t nColors = sizeof(colors) / sizeof(colors[0]);
    
    // Find global min/max for consistent binning
    Double_t globalMinE = 1e9, globalMaxE = 0;
    for (const auto& res : results) {
        for (const auto& e : res.tofEnergyValues) {
            if (e < globalMinE) globalMinE = e;
            if (e > globalMaxE) globalMaxE = e;
        }
    }
    
    if (globalMaxE > globalMinE) {
        Double_t range = globalMaxE - globalMinE;
        Double_t lowEdge = TMath::Max(0.0, globalMinE - 0.1 * range);
        Double_t highEdge = globalMaxE + 0.1 * range;
        
        std::vector<TH1D*> angleHistos;
        Double_t maxCounts = 0;
        
        for (Int_t i = 0; i < nPoints; i++) {
            TString histName = Form("hTOF_angle_%d", i);
            TString histTitle = Form("%.1f deg", angles[i]);
            
            TH1D* h = new TH1D(histName, histTitle, 100, lowEdge, highEdge);
            h->SetLineColor(colors[i % nColors]);
            h->SetLineWidth(2);
            
            for (const auto& e : results[i].tofEnergyValues) {
                h->Fill(e);
            }
            
            if (h->GetMaximum() > maxCounts) {
                maxCounts = h->GetMaximum();
            }
            
            angleHistos.push_back(h);
            leg->AddEntry(h, histTitle, "l");
        }
        
        // Draw histograms
        Bool_t first = true;
        for (auto* h : angleHistos) {
            h->SetMaximum(maxCounts * 1.1);
            if (first) {
                h->SetTitle("TOF Energy Spectrum by Detector Angle;TOF Energy [keV];Counts");
                h->Draw("HIST");
                first = false;
            } else {
                h->Draw("HIST SAME");
            }
        }
        
        leg->Draw();
    }
    
    //========================================================================//
    // Canvas 6: Generated Neutron Energy Distribution (first file only)
    //========================================================================//
    // Use only first file - distributions should be identical across all simulations
    TCanvas* c6 = new TCanvas("c6_neutron_energy", "Generated Neutron Energy Distribution", 800, 600);
    c6->SetGrid();
    
    if (!results.empty() && !results[0].neutronEnergyValues.empty()) {
        const std::vector<Double_t>& energies = results[0].neutronEnergyValues;
        
        // Determine histogram range from data
        Double_t minE = *std::min_element(energies.begin(), energies.end());
        Double_t maxE = *std::max_element(energies.begin(), energies.end());
        
        // Add padding
        Double_t range = maxE - minE;
        Double_t lowEdge = TMath::Max(0.0, minE - 0.1 * range);
        Double_t highEdge = maxE + 0.1 * range;
        
        TH1D* hNeutronEnergy = new TH1D("hNeutronEnergy",
            "Generated Neutron Energy Distribution (First File);Neutron Energy [keV];Counts",
            100, lowEdge, highEdge);
        
        hNeutronEnergy->SetFillColor(kBlue);
        hNeutronEnergy->SetFillStyle(3004);
        hNeutronEnergy->SetLineColor(kBlue);
        hNeutronEnergy->SetLineWidth(2);
        
        for (const auto& e : energies) {
            hNeutronEnergy->Fill(e);
        }
        
        hNeutronEnergy->Draw("HIST");
        
        std::cout << "Generated Neutron Energy Statistics (First File: " << results[0].filename << "):\n";
        std::cout << "  Total events: " << energies.size() << "\n";
        std::cout << "  Mean Energy: " << hNeutronEnergy->GetMean() << " keV\n";
        std::cout << "  RMS: " << hNeutronEnergy->GetRMS() << " keV\n\n";
    }
    else {
        std::cout << "Warning: No neutron energy data to histogram.\n\n";
    }
    
    //========================================================================//
    // Canvas 7: Generated Neutron Angular Distribution (first file only)
    //========================================================================//
    TCanvas* c7 = new TCanvas("c7_neutron_theta", "Generated Neutron Angular Distribution", 800, 600);
    c7->SetGrid();
    
    if (!results.empty() && !results[0].neutronThetaValues.empty()) {
        const std::vector<Double_t>& thetas = results[0].neutronThetaValues;
        
        // Determine histogram range from data
        Double_t minT = *std::min_element(thetas.begin(), thetas.end());
        Double_t maxT = *std::max_element(thetas.begin(), thetas.end());
        
        // Use 0 to 90 deg or data range, whichever is larger
        Double_t lowEdge = TMath::Min(0.0, minT);
        Double_t highEdge = TMath::Max(90.0, maxT);
        
        TH1D* hNeutronTheta = new TH1D("hNeutronTheta",
            "Generated Neutron Angular Distribution (First File);Neutron Angle [deg];Counts",
            90, lowEdge, highEdge);
        
        hNeutronTheta->SetFillColor(kRed);
        hNeutronTheta->SetFillStyle(3004);
        hNeutronTheta->SetLineColor(kRed);
        hNeutronTheta->SetLineWidth(2);
        
        for (const auto& t : thetas) {
            hNeutronTheta->Fill(t);
        }
        
        hNeutronTheta->Draw("HIST");
        
        std::cout << "Generated Neutron Angular Statistics (First File):\n";
        std::cout << "  Total events: " << thetas.size() << "\n";
        std::cout << "  Mean Angle: " << hNeutronTheta->GetMean() << " deg\n";
        std::cout << "  RMS: " << hNeutronTheta->GetRMS() << " deg\n\n";
    }
    else {
        std::cout << "Warning: No neutron theta data to histogram.\n\n";
    }
    
    //========================================================================//
    // Canvas 8: Neutron Energy vs Angle Scatterplot (first file, sampled)
    //========================================================================//
    TCanvas* c8 = new TCanvas("c8_energy_vs_theta", "Neutron Energy vs Angle", 800, 600);
    c8->SetGrid();
    
    if (!results.empty() && !results[0].neutronEnergyValues.empty()) {
        const std::vector<Double_t>& energies = results[0].neutronEnergyValues;
        const std::vector<Double_t>& thetas = results[0].neutronThetaValues;
        
        // Systematic sampling: limit to maxSamplePoints for efficiency
        const Int_t maxSamplePoints = 10000;
        Long64_t nTotal = energies.size();
        Long64_t step = (nTotal > maxSamplePoints) ? (nTotal / maxSamplePoints) : 1;
        Int_t nSampled = (nTotal > maxSamplePoints) ? maxSamplePoints : nTotal;
        
        std::vector<Double_t> sampledEnergies;
        std::vector<Double_t> sampledThetas;
        sampledEnergies.reserve(nSampled);
        sampledThetas.reserve(nSampled);
        
        for (Long64_t i = 0; i < nTotal; i += step) {
            sampledEnergies.push_back(energies[i]);
            sampledThetas.push_back(thetas[i]);
        }
        
        TGraph* gScatter = new TGraph(sampledEnergies.size(), 
                                       sampledThetas.data(), sampledEnergies.data());
        
        gScatter->SetTitle("Generated Neutron Energy vs Angle (First File, Sampled);Neutron Angle [deg];Neutron Energy [keV]");
        gScatter->SetMarkerStyle(6);  // Small dot
        gScatter->SetMarkerColor(kBlue);
        gScatter->SetMarkerSize(0.5);
        gScatter->Draw("AP");
        
        std::cout << "Scatterplot Statistics (First File):\n";
        std::cout << "  Total events: " << nTotal << "\n";
        std::cout << "  Sampled points: " << sampledEnergies.size() << "\n";
        std::cout << "  Sample step: " << step << "\n\n";
    }
    else {
        std::cout << "Warning: No data for scatterplot.\n\n";
    }
    
    //========================================================================//
    // Save plots
    //========================================================================//
    c1->SaveAs("efficiency_vs_angle.png");
    c2->SaveAs("tof_energy_vs_angle.png");
    c3->SaveAs("captures_histogram.png");
    c4->SaveAs("tof_energy_spectrum.png");
    c5->SaveAs("tof_energy_per_angle.png");
    c6->SaveAs("neutron_energy_distribution.png");
    c7->SaveAs("neutron_theta_distribution.png");
    c8->SaveAs("neutron_energy_vs_theta.png");
    
    std::cout << "Plots saved:\n";
    std::cout << "  efficiency_vs_angle.png\n";
    std::cout << "  tof_energy_vs_angle.png\n";
    std::cout << "  captures_histogram.png\n";
    std::cout << "  tof_energy_spectrum.png\n";
    std::cout << "  tof_energy_per_angle.png\n";
    std::cout << "  neutron_energy_distribution.png\n";
    std::cout << "  neutron_theta_distribution.png\n";
    std::cout << "  neutron_energy_vs_theta.png\n";
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
