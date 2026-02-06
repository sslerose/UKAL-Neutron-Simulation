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
#include "TH2D.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLine.h"
#include "TF1.h"
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

    // Energy ratio statistics (TOF / Generated for captured neutrons)
    Double_t meanEnergyRatio;     // Mean of TOF/Generated energy ratio

    // Histogram result (for per-angle TOF spectrum)
    ROOT::RDF::RResultPtr<TH1D> histTOF;

    // Fine-binned histograms for Feinberg-style analysis (1 keV bins, 0-150 keV)
    ROOT::RDF::RResultPtr<TH1D> histTOF_fine;        // TOF energy (captures only)
    ROOT::RDF::RResultPtr<TH1D> histGenEnergy_fine;  // Generated energy (entries)
    ROOT::RDF::RResultPtr<TH1D> histTOFTime_fine;    // TOF time (captures only)

    // Diagnostic histograms for energy calculation verification
    ROOT::RDF::RResultPtr<TH2D> histGenVsTOF;        // 2D: Generated vs TOF energy
    ROOT::RDF::RResultPtr<TH1D> histEnergyDiff;      // Generated - TOF energy
    ROOT::RDF::RResultPtr<TH1D> histEnergyRatio;     // TOF / Generated energy ratio

    AngleResults()
        : angle(-999.0), distance(-999.0), totalEvents(0), entries(0), captures(0),
          efficiency(0.0), efficiencyError(0.0),
          meanTOFEnergy(0.0), stddevTOFEnergy(0.0), stderrTOFEnergy(0.0),
          meanGenEnergy(0.0), stddevGenEnergy(0.0), stderrGenEnergy(0.0),
          meanEnergyRatio(0.0) {}
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
    // Fine-binned histograms for Feinberg-style analysis (1 keV bins, 0-150 keV)
    //========================================================================//
    // TOF energy histogram (captures only) - fine binning
    auto histNameTOF_fine = Form("hTOF_fine_angle_%.1f", angle);
    auto histTOF_fine = dfCapture.Histo1D(
        {histNameTOF_fine, histNameTOF_fine, 150, 0, 150}, "TOFEnergy");

    // Generated energy histogram for neutrons that ENTERED detector (better statistics)
    auto histNameGen_fine = Form("hGenEnergy_fine_angle_%.1f", angle);
    auto histGenEnergy_fine = dfEntered.Histo1D(
        {histNameGen_fine, histNameGen_fine, 150, 0, 150}, "NeutronEnergy");

    // TOF time histogram (captures only) - for diagnostic comparison with energy
    // Binning: 0-300 ns with 1 ns bins
    auto histNameTOFTime_fine = Form("hTOFTime_fine_angle_%.1f", angle);
    auto histTOFTime_fine = dfCapture.Histo1D(
        {histNameTOFTime_fine, histNameTOFTime_fine, 300, 0, 300}, "TOF");

    //========================================================================//
    // Diagnostic histograms for energy calculation verification
    //========================================================================//
    // 2D histogram: Generated Energy vs TOF Energy
    auto histNameGenVsTOF = Form("hGenVsTOF_angle_%.1f", angle);
    auto histGenVsTOF = dfCapture.Histo2D(
        {histNameGenVsTOF, histNameGenVsTOF, 150, 0, 150, 150, 0, 150},
        "NeutronEnergy", "TOFEnergy");

    // Energy difference: Generated - TOF (should be centered near 0 if calculation is correct)
    auto dfCaptureWithDiff = dfCapture.Define("EnergyDiff", "NeutronEnergy - TOFEnergy");
    auto histNameEnergyDiff = Form("hEnergyDiff_angle_%.1f", angle);
    auto histEnergyDiff = dfCaptureWithDiff.Histo1D(
        {histNameEnergyDiff, histNameEnergyDiff, 200, -100, 100}, "EnergyDiff");

    // Energy ratio: TOF / Generated (should be centered near 1.0 if calculation is correct)
    auto dfCaptureWithRatio = dfCapture.Define("EnergyRatio",
        "NeutronEnergy > 0 ? TOFEnergy / NeutronEnergy : -1.0");
    auto histNameEnergyRatio = Form("hEnergyRatio_angle_%.1f", angle);
    auto histEnergyRatio = dfCaptureWithRatio.Filter("EnergyRatio > 0").Histo1D(
        {histNameEnergyRatio, histNameEnergyRatio, 200, 0, 3}, "EnergyRatio");

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

    // Calculate mean energy ratio (TOF/Generated) for captured neutrons
    if (result.captures > 0) {
        result.meanEnergyRatio = histEnergyRatio->GetMean();
    }

    // Store histogram result pointers (not evaluated yet)
    result.histTOF = histTOF;
    result.histTOF_fine = histTOF_fine;
    result.histGenEnergy_fine = histGenEnergy_fine;
    result.histTOFTime_fine = histTOFTime_fine;
    result.histGenVsTOF = histGenVsTOF;
    result.histEnergyDiff = histEnergyDiff;
    result.histEnergyRatio = histEnergyRatio;

    return result;
}

//============================================================================//
// Load efficiency fit parameters from file generated by AnalyzeEff.C
// Returns a TF1* with the polynomial fit, or nullptr if file not found
//============================================================================//
TF1* loadEfficiencyFit(const char* fitFile = "efficiency_fit.txt") {
    std::ifstream inFile(fitFile);
    if (!inFile.is_open()) {
        std::cerr << "Warning: Cannot open efficiency fit file: " << fitFile << std::endl;
        std::cerr << "  Feinberg angular spectra will use normalized counts (no efficiency correction).\n";
        return nullptr;
    }

    Int_t degree = -1;
    std::vector<Double_t> params;
    std::string line;

    while (std::getline(inFile, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "DEGREE") {
            iss >> degree;
            params.resize(degree + 1, 0.0);
        } else if (key.length() >= 2 && key[0] == 'p') {
            Int_t idx = std::stoi(key.substr(1));
            Double_t val;
            iss >> val;
            if (idx >= 0 && idx <= degree) {
                params[idx] = val;
            }
        }
    }
    inFile.close();

    if (degree < 0 || params.empty()) {
        std::cerr << "Warning: Failed to parse efficiency fit from: " << fitFile << std::endl;
        return nullptr;
    }

    // Create TF1 with the polynomial
    TString formula = Form("pol%d", degree);
    TF1* effFunc = new TF1("efficiencyFit", formula, 0.0, 150.0);
    for (Int_t i = 0; i <= degree; i++) {
        effFunc->SetParameter(i, params[i]);
    }

    std::cout << "Loaded efficiency fit (pol" << degree << ") from " << fitFile << std::endl;
    return effFunc;
}

//============================================================================//
// Feinberg-style individual angular spectra (Figure 37/41 style)
// Plots generated vs detected (TOF) energy spectra for selected angles
// When efficiency function is provided, TOF spectrum is converted to
// efficiency-corrected double-differential d²N/(dE dΩ)
//============================================================================//
void plotFeinbergAngularSpectra(std::vector<AngleResults>& results,
                                 const std::vector<Double_t>& targetAngles = {10.0, 30.0, 50.0, 60.0},
                                 TF1* effFunc = nullptr) {

    // Find which target angles are available in the data
    std::vector<AngleResults*> matchedResults;
    std::vector<Double_t> matchedAngles;

    for (Double_t targetAngle : targetAngles) {
        AngleResults* bestMatch = nullptr;
        Double_t minDiff = 999.0;

        for (auto& res : results) {
            Double_t diff = TMath::Abs(res.angle - targetAngle);
            if (diff < minDiff) {
                minDiff = diff;
                bestMatch = &res;
            }
        }

        // Only include if within 2.5 degrees of target (half a typical 5-deg bin)
        if (bestMatch && minDiff <= 2.5) {
            matchedResults.push_back(bestMatch);
            matchedAngles.push_back(bestMatch->angle);
        }
    }

    if (matchedResults.empty()) {
        std::cerr << "Warning: No matching angles found for Feinberg angular spectra plot.\n";
        return;
    }

    // Determine canvas layout based on number of matched angles
    Int_t nPlots = matchedResults.size();
    Int_t nCols = (nPlots <= 2) ? nPlots : 2;
    Int_t nRows = (nPlots + nCols - 1) / nCols;

    TCanvas* cFeinberg = new TCanvas("cFeinberg_angular",
        "Feinberg-Style Angular Spectra", 600 * nCols, 500 * nRows);
    cFeinberg->Divide(nCols, nRows);

    for (size_t idx = 0; idx < matchedResults.size(); idx++) {
        AngleResults* res = matchedResults[idx];

        cFeinberg->cd(idx + 1);
        gPad->SetGrid();
        gPad->SetLeftMargin(effFunc ? 0.14 : 0.12);
        gPad->SetRightMargin(0.05);

        // Clone histograms to avoid modifying originals
        TH1D* hTOF = (TH1D*)res->histTOF_fine->Clone(
            Form("hTOF_feinberg_%.1f", res->angle));
        TH1D* hGen = (TH1D*)res->histGenEnergy_fine->Clone(
            Form("hGen_feinberg_%.1f", res->angle));

        if (effFunc) {
            //================================================================//
            // Efficiency-corrected double-differential spectrum
            // d²N/(dE dΩ) = N_cap(E) / (ε(E_center) × ΔE × ΔΩ)
            //================================================================//

            // Solid angle: ΔΩ = π r² / d², r = 2.54 cm (detector radius)
            Double_t detRadius = 2.54;  // cm (Constants.hh: kDetectorRadius = 5.08 cm / 2)
            Double_t d = res->distance; // cm
            Double_t deltaOmega = TMath::Pi() * detRadius * detRadius / (d * d);

            // Apply efficiency correction bin-by-bin to TOF histogram
            for (Int_t bin = 1; bin <= hTOF->GetNbinsX(); bin++) {
                Double_t binCenter = hTOF->GetBinCenter(bin);
                Double_t deltaE = hTOF->GetBinWidth(bin);
                Double_t counts = hTOF->GetBinContent(bin);
                Double_t epsilon = effFunc->Eval(binCenter);

                if (epsilon > 1e-6) {
                    Double_t corrected = counts / (epsilon * deltaE * deltaOmega);
                    hTOF->SetBinContent(bin, corrected);
                    Double_t error = (counts > 0) ?
                        TMath::Sqrt(counts) / (epsilon * deltaE * deltaOmega) : 0;
                    hTOF->SetBinError(bin, error);
                } else {
                    hTOF->SetBinContent(bin, 0);
                    hTOF->SetBinError(bin, 0);
                }
            }

            // Normalize generated spectrum to unit integral, then scale
            // so its peak matches the corrected TOF peak for shape comparison
            Double_t integralGen = hGen->Integral();
            if (integralGen > 0) hGen->Scale(1.0 / integralGen);

            Double_t maxTOF = hTOF->GetMaximum();
            Double_t maxGen = hGen->GetMaximum();
            if (maxGen > 0) hGen->Scale(maxTOF / maxGen);

            // Style for generated spectrum (filled, semi-transparent)
            hGen->SetFillColor(kBlue);
            hGen->SetFillStyle(3004);
            hGen->SetLineColor(kBlue);
            hGen->SetLineWidth(2);

            // Style for TOF spectrum (line only, overlaid)
            hTOF->SetLineColor(kRed);
            hTOF->SetLineWidth(2);
            hTOF->SetFillStyle(0);

            // Set axis labels
            hGen->SetTitle(Form("#theta = %.1f#circ;Energy [keV];d^{2}N/(dE d#Omega) [counts/(keV sr)]",
                                res->angle));
            hGen->SetStats(0);
            hGen->GetYaxis()->SetTitleOffset(1.6);

            // Find maximum for Y axis scaling
            Double_t maxY = TMath::Max(hGen->GetMaximum(), hTOF->GetMaximum());
            hGen->SetMaximum(maxY * 1.2);
            hGen->SetMinimum(0);

            // Draw
            hGen->Draw("HIST");
            hTOF->Draw("HIST SAME");

            // Add legend
            TLegend* leg = new TLegend(0.45, 0.72, 0.93, 0.88);
            leg->AddEntry(hGen, "Generated (scaled)", "f");
            leg->AddEntry(hTOF, "Detected (eff-corrected)", "l");
            leg->SetBorderSize(0);
            leg->Draw();

        } else {
            //================================================================//
            // Fallback: original normalized-counts behavior
            //================================================================//

            // Normalize both histograms to same integral for shape comparison
            Double_t integralTOF = hTOF->Integral();
            Double_t integralGen = hGen->Integral();

            if (integralTOF > 0) hTOF->Scale(1.0 / integralTOF);
            if (integralGen > 0) hGen->Scale(1.0 / integralGen);

            // Style for generated spectrum (filled, semi-transparent)
            hGen->SetFillColor(kBlue);
            hGen->SetFillStyle(3004);
            hGen->SetLineColor(kBlue);
            hGen->SetLineWidth(2);

            // Style for TOF spectrum (line only, overlaid)
            hTOF->SetLineColor(kRed);
            hTOF->SetLineWidth(2);
            hTOF->SetFillStyle(0);

            // Set axis labels
            hGen->SetTitle(Form("#theta = %.1f#circ;Energy [keV];Normalized Counts / keV",
                                res->angle));
            hGen->SetStats(0);
            hGen->GetYaxis()->SetTitleOffset(1.4);

            // Find maximum for Y axis scaling
            Double_t maxY = TMath::Max(hGen->GetMaximum(), hTOF->GetMaximum());
            hGen->SetMaximum(maxY * 1.2);
            hGen->SetMinimum(0);

            // Draw
            hGen->Draw("HIST");
            hTOF->Draw("HIST SAME");

            // Add legend
            TLegend* leg = new TLegend(0.55, 0.75, 0.93, 0.88);
            leg->AddEntry(hGen, "Generated (entries)", "f");
            leg->AddEntry(hTOF, "Detected (TOF)", "l");
            leg->SetBorderSize(0);
            leg->Draw();
        }
    }

    cFeinberg->Update();
    cFeinberg->SaveAs("feinberg_angular_spectra.png");

    std::cout << "Feinberg angular spectra created for angles: ";
    for (size_t i = 0; i < matchedAngles.size(); i++) {
        std::cout << matchedAngles[i];
        if (i < matchedAngles.size() - 1) std::cout << ", ";
    }
    std::cout << " degrees\n";
}

//============================================================================//
// TOF Time Spectra for Diagnostic Comparison (similar to Feinberg angular)
// Plots raw time-of-flight spectra at selected angles
// Normalized to unit integral for shape comparison with energy spectra
// Purpose: Identify potential issues with energy calculation from TOF
//============================================================================//
void plotTOFTimeSpectra(std::vector<AngleResults>& results,
                        const std::vector<Double_t>& targetAngles = {10.0, 30.0, 50.0, 60.0}) {

    // Find which target angles are available in the data
    std::vector<AngleResults*> matchedResults;
    std::vector<Double_t> matchedAngles;

    for (Double_t targetAngle : targetAngles) {
        AngleResults* bestMatch = nullptr;
        Double_t minDiff = 999.0;

        for (auto& res : results) {
            Double_t diff = TMath::Abs(res.angle - targetAngle);
            if (diff < minDiff) {
                minDiff = diff;
                bestMatch = &res;
            }
        }

        // Only include if within 2.5 degrees of target (half a typical 5-deg bin)
        if (bestMatch && minDiff <= 2.5) {
            matchedResults.push_back(bestMatch);
            matchedAngles.push_back(bestMatch->angle);
        }
    }

    if (matchedResults.empty()) {
        std::cerr << "Warning: No matching angles found for TOF time spectra plot.\n";
        return;
    }

    // Determine canvas layout based on number of matched angles
    Int_t nPlots = matchedResults.size();
    Int_t nCols = (nPlots <= 2) ? nPlots : 2;
    Int_t nRows = (nPlots + nCols - 1) / nCols;

    TCanvas* cTOFTime = new TCanvas("cTOFTime_diagnostic",
        "TOF Time Spectra (Diagnostic)", 600 * nCols, 500 * nRows);
    cTOFTime->Divide(nCols, nRows);

    for (size_t idx = 0; idx < matchedResults.size(); idx++) {
        AngleResults* res = matchedResults[idx];

        cTOFTime->cd(idx + 1);
        gPad->SetGrid();
        gPad->SetLeftMargin(0.12);
        gPad->SetRightMargin(0.05);

        // Clone histogram to avoid modifying original
        TH1D* hTime = (TH1D*)res->histTOFTime_fine->Clone(
            Form("hTOFTime_diag_%.1f", res->angle));

        // Normalize to unit integral for shape comparison
        Double_t integral = hTime->Integral();
        if (integral > 0) hTime->Scale(1.0 / integral);

        // Style for TOF time spectrum
        hTime->SetLineColor(kBlue);
        hTime->SetLineWidth(2);
        hTime->SetFillColor(kBlue);
        hTime->SetFillStyle(3004);

        // Set axis labels
        hTime->SetTitle(Form("#theta = %.1f#circ;TOF Time [ns];Normalized Counts / ns",
                            res->angle));
        hTime->SetStats(0);
        hTime->GetYaxis()->SetTitleOffset(1.4);

        // Set Y axis range
        hTime->SetMinimum(0);
        hTime->SetMaximum(hTime->GetMaximum() * 1.2);

        // Draw
        hTime->Draw("HIST");

        // Add text box with statistics
        TLegend* leg = new TLegend(0.55, 0.75, 0.93, 0.88);
        leg->AddEntry(hTime, Form("Captures: %llu", res->captures), "f");
        leg->SetBorderSize(0);
        leg->Draw();
    }

    cTOFTime->Update();
    cTOFTime->SaveAs("tof_time_spectra_diagnostic.png");

    std::cout << "TOF time spectra (diagnostic) created for angles: ";
    for (size_t i = 0; i < matchedAngles.size(); i++) {
        std::cout << matchedAngles[i];
        if (i < matchedAngles.size() - 1) std::cout << ", ";
    }
    std::cout << " degrees\n";
}

//============================================================================//
// Energy Calculation Diagnostic Plots
// Creates scatter plots and histograms to verify TOF energy calculation
// - 2D scatter: Generated vs TOF energy (should lie on y=x line)
// - Energy difference histogram (should be centered at 0)
// - Energy ratio histogram (should be centered at 1.0)
//============================================================================//
void plotEnergyDiagnostics(std::vector<AngleResults>& results, std::ofstream& outFile) {

    if (results.empty()) {
        std::cerr << "Warning: No results for energy diagnostics.\n";
        return;
    }

    // Helper lambda for dual output (console + file)
    auto printBoth = [&outFile](const std::string& msg) {
        std::cout << msg;
        if (outFile.is_open()) outFile << msg;
    };

    printBoth("\n=== Energy Calculation Diagnostics ===\n\n");

    // Select a few representative angles for detailed plots
    std::vector<Double_t> targetAngles = {10.0, 30.0, 60.0};
    std::vector<AngleResults*> selectedResults;

    for (Double_t targetAngle : targetAngles) {
        for (auto& res : results) {
            if (TMath::Abs(res.angle - targetAngle) <= 2.5) {
                selectedResults.push_back(&res);
                break;
            }
        }
    }

    if (selectedResults.empty()) {
        printBoth("Warning: No matching angles found for energy diagnostics.\n");
        return;
    }

    //========================================================================//
    // Canvas 1: Generated vs TOF Energy Scatter Plots
    //========================================================================//
    Int_t nPlots = selectedResults.size();
    TCanvas* cScatter = new TCanvas("cEnergy_scatter",
        "Generated vs TOF Energy", 600 * nPlots, 600);
    cScatter->Divide(nPlots, 1);

    for (size_t idx = 0; idx < selectedResults.size(); idx++) {
        AngleResults* res = selectedResults[idx];
        cScatter->cd(idx + 1);
        gPad->SetGrid();
        gPad->SetLeftMargin(0.12);
        gPad->SetRightMargin(0.12);

        TH2D* h2D = (TH2D*)res->histGenVsTOF->Clone(
            Form("hGenVsTOF_diag_%.1f", res->angle));

        h2D->SetTitle(Form("#theta = %.1f#circ;Generated Energy [keV];TOF Energy [keV]",
                          res->angle));
        h2D->SetStats(0);
        h2D->Draw("COLZ");

        // Draw y=x reference line
        TGraph* refLine = new TGraph(2);
        refLine->SetPoint(0, 0, 0);
        refLine->SetPoint(1, 150, 150);
        refLine->SetLineColor(kRed);
        refLine->SetLineWidth(2);
        refLine->SetLineStyle(2);
        refLine->Draw("L SAME");

        // Add legend
        TLegend* leg = new TLegend(0.15, 0.75, 0.45, 0.88);
        leg->AddEntry(refLine, "Perfect agreement", "l");
        leg->SetBorderSize(0);
        leg->Draw();
    }

    cScatter->Update();
    cScatter->SaveAs("energy_diagnostic_scatter.png");

    //========================================================================//
    // Canvas 2: Energy Difference Histograms (Gen - TOF)
    //========================================================================//
    TCanvas* cDiff = new TCanvas("cEnergy_diff",
        "Energy Difference (Generated - TOF)", 600 * nPlots, 500);
    cDiff->Divide(nPlots, 1);

    for (size_t idx = 0; idx < selectedResults.size(); idx++) {
        AngleResults* res = selectedResults[idx];
        cDiff->cd(idx + 1);
        gPad->SetGrid();
        gPad->SetLeftMargin(0.12);

        TH1D* hDiff = (TH1D*)res->histEnergyDiff->Clone(
            Form("hDiff_diag_%.1f", res->angle));

        hDiff->SetTitle(Form("#theta = %.1f#circ;E_{Gen} - E_{TOF} [keV];Counts",
                            res->angle));
        hDiff->SetStats(1);
        hDiff->SetLineColor(kBlue);
        hDiff->SetLineWidth(2);
        hDiff->SetFillColor(kBlue);
        hDiff->SetFillStyle(3004);
        hDiff->Draw("HIST");

        // Draw vertical line at 0
        TLine* zeroLine = new TLine(0, 0, 0, hDiff->GetMaximum());
        zeroLine->SetLineColor(kRed);
        zeroLine->SetLineWidth(2);
        zeroLine->SetLineStyle(2);
        zeroLine->Draw();
    }

    cDiff->Update();
    cDiff->SaveAs("energy_diagnostic_difference.png");

    //========================================================================//
    // Canvas 3: Energy Ratio Histograms (TOF / Gen)
    //========================================================================//
    TCanvas* cRatio = new TCanvas("cEnergy_ratio",
        "Energy Ratio (TOF / Generated)", 600 * nPlots, 500);
    cRatio->Divide(nPlots, 1);

    for (size_t idx = 0; idx < selectedResults.size(); idx++) {
        AngleResults* res = selectedResults[idx];
        cRatio->cd(idx + 1);
        gPad->SetGrid();
        gPad->SetLeftMargin(0.12);

        TH1D* hRatio = (TH1D*)res->histEnergyRatio->Clone(
            Form("hRatio_diag_%.1f", res->angle));

        hRatio->SetTitle(Form("#theta = %.1f#circ;E_{TOF} / E_{Gen};Counts",
                             res->angle));
        hRatio->SetStats(1);
        hRatio->SetLineColor(kGreen+2);
        hRatio->SetLineWidth(2);
        hRatio->SetFillColor(kGreen+2);
        hRatio->SetFillStyle(3004);
        hRatio->Draw("HIST");

        // Draw vertical line at 1.0
        TLine* unityLine = new TLine(1.0, 0, 1.0, hRatio->GetMaximum());
        unityLine->SetLineColor(kRed);
        unityLine->SetLineWidth(2);
        unityLine->SetLineStyle(2);
        unityLine->Draw();
    }

    cRatio->Update();
    cRatio->SaveAs("energy_diagnostic_ratio.png");

    //========================================================================//
    // Print statistics to file
    //========================================================================//
    printBoth("Per-Angle Energy Calculation Statistics:\n");
    printBoth("--------------------------------------------------------------------------------\n");
    printBoth(Form("%-10s %-12s %-15s %-15s %-15s\n",
                   "Angle", "Captures", "Mean(G-T)", "RMS(G-T)", "Mean(T/G)"));
    printBoth(Form("%-10s %-12s %-15s %-15s %-15s\n",
                   "[deg]", "", "[keV]", "[keV]", ""));
    printBoth("--------------------------------------------------------------------------------\n");

    for (auto& res : results) {
        if (res.captures > 0) {
            Double_t meanDiff = res.histEnergyDiff->GetMean();
            Double_t rmsDiff = res.histEnergyDiff->GetRMS();
            Double_t meanRatio = res.histEnergyRatio->GetMean();

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1)
                << std::setw(10) << res.angle
                << std::setw(12) << res.captures
                << std::setprecision(3)
                << std::setw(15) << meanDiff
                << std::setw(15) << rmsDiff
                << std::setw(15) << meanRatio
                << "\n";
            printBoth(oss.str());
        }
    }

    printBoth("--------------------------------------------------------------------------------\n");
    printBoth("Interpretation:\n");
    printBoth("  - Mean(G-T) should be near 0 if energy calculation is unbiased\n");
    printBoth("  - RMS(G-T) indicates spread/uncertainty in energy measurement\n");
    printBoth("  - Mean(T/G) should be near 1.0 for correct calculation\n");
    printBoth("  - Large deviations indicate systematic errors in TOF->Energy conversion\n\n");

    printBoth("Energy diagnostic plots saved:\n");
    printBoth("  - energy_diagnostic_scatter.png (2D scatter plots)\n");
    printBoth("  - energy_diagnostic_difference.png (difference histograms)\n");
    printBoth("  - energy_diagnostic_ratio.png (ratio histograms)\n\n");
}

//============================================================================//
// Feinberg-style solid-angle weighted integral spectrum (Figure 38 style)
// Sums spectra from all angles with weight = sin(theta_center)
// The constant 2*pi*dtheta cancels in normalization
//============================================================================//
void plotFeinbergIntegralSpectrum(std::vector<AngleResults>& results, std::ofstream& outFile) {

    if (results.empty()) {
        std::cerr << "Warning: No results for Feinberg integral spectrum.\n";
        return;
    }

    // Helper lambda for dual output (console + file)
    auto printBoth = [&outFile](const std::string& msg) {
        std::cout << msg;
        if (outFile.is_open()) outFile << msg;
    };

    // Create output histograms for weighted sum (1 keV bins, 0-150 keV)
    TH1D* hWeightedTOF = new TH1D("hWeightedTOF_integral",
        "Solid-Angle Weighted TOF Spectrum", 150, 0, 150);
    TH1D* hWeightedGen = new TH1D("hWeightedGen_integral",
        "Solid-Angle Weighted Generated Spectrum", 150, 0, 150);

    printBoth("\nSolid Angle Weights Applied (weight = sin(theta)):\n");

    // Sum contributions from all angles with solid angle weighting
    for (auto& res : results) {
        // Calculate solid angle weight: sin(theta_center)
        // The factor 2*pi*dtheta is constant and cancels in normalization
        Double_t thetaRad = TMath::Abs(res.angle) * TMath::DegToRad();
        Double_t weight = TMath::Sin(thetaRad);

        // Handle theta=0 case (weight would be 0)
        if (weight < 1e-10) {
            std::ostringstream oss;
            oss << "  Angle " << std::setw(6) << std::fixed << std::setprecision(1)
                << res.angle << " deg: weight = " << std::setprecision(4)
                << weight << " (skipped, zero weight)\n";
            printBoth(oss.str());
            continue;
        }

        std::ostringstream oss;
        oss << "  Angle " << std::setw(6) << std::fixed << std::setprecision(1)
            << res.angle << " deg: weight = " << std::setprecision(4) << weight << "\n";
        printBoth(oss.str());

        // Get fine-binned histograms and add weighted contribution
        TH1D* hTOF = (TH1D*)res.histTOF_fine->Clone();
        TH1D* hGen = (TH1D*)res.histGenEnergy_fine->Clone();

        hWeightedTOF->Add(hTOF, weight);
        hWeightedGen->Add(hGen, weight);

        delete hTOF;
        delete hGen;
    }

    // Create canvas with single panel: normalized spectrum
    TCanvas* cIntegral = new TCanvas("cFeinberg_integral",
        "Feinberg-Style Integral Spectrum", 800, 600);

    //--- Normalized spectrum for shape comparison ---
    gPad->SetGrid();
    gPad->SetLeftMargin(0.12);

    TH1D* hGenNorm = (TH1D*)hWeightedGen->Clone("hGen_norm");
    TH1D* hTOFNorm = (TH1D*)hWeightedTOF->Clone("hTOF_norm");

    // Normalize to unit integral
    Double_t intGen = hGenNorm->Integral();
    Double_t intTOF = hTOFNorm->Integral();
    if (intGen > 0) hGenNorm->Scale(1.0 / intGen);
    if (intTOF > 0) hTOFNorm->Scale(1.0 / intTOF);

    hGenNorm->SetFillColor(kBlue);
    hGenNorm->SetFillStyle(3004);
    hGenNorm->SetLineColor(kBlue);
    hGenNorm->SetLineWidth(2);

    hTOFNorm->SetLineColor(kRed);
    hTOFNorm->SetLineWidth(2);
    hTOFNorm->SetFillStyle(0);

    hGenNorm->SetTitle("Solid-Angle Weighted (Normalized);Energy [keV];Normalized Counts / keV");
    hGenNorm->SetStats(0);
    hGenNorm->GetYaxis()->SetTitleOffset(1.4);

    Double_t maxNorm = TMath::Max(hGenNorm->GetMaximum(), hTOFNorm->GetMaximum());
    hGenNorm->SetMaximum(maxNorm * 1.2);
    hGenNorm->SetMinimum(0);

    hGenNorm->Draw("HIST");
    hTOFNorm->Draw("HIST SAME");

    TLegend* legNorm = new TLegend(0.55, 0.75, 0.88, 0.88);
    legNorm->AddEntry(hGenNorm, "Generated (norm)", "f");
    legNorm->AddEntry(hTOFNorm, "Detected (norm)", "l");
    legNorm->SetBorderSize(0);
    legNorm->Draw();

    cIntegral->Update();
    cIntegral->SaveAs("feinberg_integral_spectrum.png");

    // Cleanup base histograms
    delete hWeightedTOF;
    delete hWeightedGen;

    printBoth("Feinberg integral spectrum saved.\n");
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
    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    tableStream << std::setw(10) << "Angle"
                << std::setw(10) << "Distance"
                << std::setw(12) << "Events"
                << std::setw(12) << "Entries"
                << std::setw(12) << "Captures"
                << std::setw(18) << "Efficiency"
                << std::setw(22) << "Gen Energy"
                << std::setw(22) << "TOF Energy"
                << std::setw(12) << "Mean(T/G)"
                << "\n";
    tableStream << std::setw(10) << "[deg]"
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
                << std::setw(11) << std::setprecision(3) << 100.0 * totalCapturesAll / totalEntriesAll
                << "\n";
    tableStream << "============================================================================================================================================================\n\n";

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

    //========================================================================//
    // Feinberg-style analysis plots
    //========================================================================//
    printBoth("\n--- Feinberg-Style Analysis ---\n");

    // Load efficiency fit if available (generated by AnalyzeEff.C)
    TF1* effFunc = loadEfficiencyFit("efficiency_fit.txt");

    // Plot individual angular spectra for representative angles (10, 30, 50, 60 deg)
    // If efficiency fit is available, applies correction: d²N/(dE dΩ) = N/(ε·ΔE·ΔΩ)
    plotFeinbergAngularSpectra(results, {10.0, 30.0, 50.0, 60.0}, effFunc);

    if (effFunc) {
        printBoth("  Feinberg angular spectra: efficiency-corrected d^2N/(dE dOmega)\n");
        printBoth("  Efficiency fit loaded from: efficiency_fit.txt\n");
    } else {
        printBoth("  Feinberg angular spectra: normalized counts (no efficiency correction)\n");
    }

    // Plot TOF time spectra for diagnostic comparison with energy
    plotTOFTimeSpectra(results);

    // Plot solid-angle weighted integral spectrum (weight statistics written to file)
    plotFeinbergIntegralSpectrum(results, outFile);

    printBoth("\n");

    //========================================================================//
    // Energy calculation diagnostic plots
    //========================================================================//
    plotEnergyDiagnostics(results, outFile);

    std::ostringstream plotList;
    plotList << "Plots saved:\n"
             << "  efficiency_vs_angle.png\n"
             << "  tof_energy_vs_angle.png\n"
             << "  captures_histogram.png\n"
             << "  tof_energy_spectrum.png\n"
             << "  tof_energy_per_angle.png\n"
             << "  neutron_energy_distribution.png\n"
             << "  neutron_theta_distribution.png\n"
             << "  neutron_energy_vs_theta.png\n"
             << "  feinberg_angular_spectra.png (Feinberg Fig 37/41 style"
             << (effFunc ? ", eff-corrected" : ", normalized") << ")\n"
             << "  tof_time_spectra_diagnostic.png (TOF time diagnostic)\n"
             << "  feinberg_integral_spectrum.png (Feinberg Fig 38 style)\n"
             << "  energy_diagnostic_scatter.png (Generated vs TOF energy)\n"
             << "  energy_diagnostic_difference.png (Energy difference histograms)\n"
             << "  energy_diagnostic_ratio.png (Energy ratio histograms)\n\n";
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
