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
// * https://opensource.org/license/mit .                             *                                        *
// *                                                                  *
// ********************************************************************

//
/// \file AnalyzeTOF.C
/// \brief ROOT analysis macro for neutron detection simulation data.
//
// Usage:
//   root -l
//   .L AnalyzeTOF.C
//   analyzeTOF()
//
//   Specify path to data directory:
//   analyzeTOF("path/to/data/directory")
//
//   Choose individual anglular spectra to plot
//   analyzeTOF("path/to/data/directory", {10.0, 30.0, 50.0, 60.0})
//
//   To generate diagnostic plots:
//   analyzeTOF("path/to/data/directory", includeDiagnostic = true)
//
//   To specify the number of threads for multi-threading:
//   analyzeTOF("path/to/data/directory", nThreads = 4)
//
// Note: efficiency table must be placed in the same directory as the data files, named "efficiency_table.txt".
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
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <thread>

//============================================================================//
//                              DATA STRUCTURES                               //
//============================================================================//

//----------------------------------------------------------------------------//
// Structure to hold parsed filename parameters
//----------------------------------------------------------------------------//
struct FilenameParams {
    Double_t angle;
    Double_t distance;
    Bool_t valid;

    FilenameParams() : angle(-999.0), distance(-999.0), valid(false) {}
};

//----------------------------------------------------------------------------//
// Result structure for per-angle analysis
//----------------------------------------------------------------------------//
struct AngleResults {
    TString filename;
    Double_t angle;             // Parsed angle from filename (degrees)
    Double_t distance;          // Parsed distance from filename (cm)
    ULong64_t totalEvents;
    ULong64_t entries;          // Neutrons that entered the detector
    ULong64_t captures;         // Neutrons that were captured
    Double_t efficiency;        // Capture efficiency: captures / entries
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

    // Fine-binned histograms for angular and integral analysis (1 keV bins, 0-150 keV)
    ROOT::RDF::RResultPtr<TH1D> histTOF_fine;        // TOF energy (captures only)
    ROOT::RDF::RResultPtr<TH1D> histGenEnergy_fine;  // Generated energy (entries)
    ROOT::RDF::RResultPtr<TH1D> histTOFTime_fine;    // TOF time (captures only)

    // Diagnostic histograms for energy calculation verification
    ROOT::RDF::RResultPtr<TH2D> histGenVsTOF;        // 2D: Generated vs TOF energy
    ROOT::RDF::RResultPtr<TH1D> histEnergyDiff;      // Generated - TOF energy
    ROOT::RDF::RResultPtr<TH1D> histEnergyRatio;     // TOF / Generated energy ratio

    // Full neutron distributions (populated for first angle group only)
    ROOT::RDF::RResultPtr<TH1D> histNeutronEnergy;   // All events NeutronEnergy
    ROOT::RDF::RResultPtr<TH1D> histNeutronTheta;    // All events NeutronTheta
    ROOT::RDF::RResultPtr<TH2D> histEnergyVsTheta;   // 2D: Energy vs Theta (sampled)

    AngleResults()
        : angle(-999.0), distance(-999.0), totalEvents(0), entries(0), captures(0),
          efficiency(0.0), efficiencyError(0.0),
          meanTOFEnergy(0.0), stddevTOFEnergy(0.0), stderrTOFEnergy(0.0),
          meanGenEnergy(0.0), stddevGenEnergy(0.0), stderrGenEnergy(0.0),
          meanEnergyRatio(0.0) {}
};

//----------------------------------------------------------------------------//
// Tabulated efficiency data with log-log linear interpolation
//----------------------------------------------------------------------------//
struct EfficiencyTable {
    std::vector<Double_t> energies;      // keV, sorted ascending
    std::vector<Double_t> efficiencies;  // dimensionless, 0 to 1
    std::vector<Double_t> errors;        // binomial errors

    Double_t Eval(Double_t E) const {
        if (energies.empty()) return 0.0;

        // Below range: extrapolate power law from first two points
        if (E <= energies.front()) {
            if (energies.size() >= 2 && energies[0] > 0 && energies[1] > 0 &&
                efficiencies[0] > 0 && efficiencies[1] > 0) {
                Double_t logE0 = std::log(energies[0]);
                Double_t logE1 = std::log(energies[1]);
                Double_t logEps0 = std::log(efficiencies[0]);
                Double_t logEps1 = std::log(efficiencies[1]);
                Double_t logE = std::log(TMath::Max(E, 1e-10));
                Double_t logEps = logEps0 + (logEps1 - logEps0) *
                                  (logE - logE0) / (logE1 - logE0);
                return std::exp(logEps);
            }
            return efficiencies.front();
        }

        // Above range: clamp to last data point
        if (E >= energies.back()) {
            return efficiencies.back();
        }

        // Binary search for bracketing interval
        auto it = std::upper_bound(energies.begin(), energies.end(), E);
        size_t i1 = std::distance(energies.begin(), it);
        size_t i0 = i1 - 1;

        // Log-log linear interpolation
        Double_t logE0 = std::log(energies[i0]);
        Double_t logE1 = std::log(energies[i1]);
        Double_t logEps0 = std::log(efficiencies[i0]);
        Double_t logEps1 = std::log(efficiencies[i1]);
        Double_t logE = std::log(E);

        Double_t logEps = logEps0 + (logEps1 - logEps0) *
                          (logE - logE0) / (logE1 - logE0);

        return std::exp(logEps);
    }
};

//============================================================================//
//                            UTILITY FUNCTIONS                               //
//============================================================================//

using PrintFn = std::function<void(const std::string&)>;

//----------------------------------------------------------------------------//
// Parse detector angle and distance from filename
// Expected format:
//   nTOF_SimLiT_1912_0kev_10_0deg_50_0cm.root  ->  angle=10.0, distance=50.0
//   nTOF_SimLiT_1912_0kev_-60_0deg_50_0cm.root ->  angle=-60.0, distance=50.0
//
// Returns a struct with parsed values and validity flag
//----------------------------------------------------------------------------//
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

//----------------------------------------------------------------------------//
// Load tabulated efficiency data from file generated by AnalyzeEff.C
// Returns nullptr if file not found or empty
//----------------------------------------------------------------------------//
EfficiencyTable* loadEfficiencyTable(const char* tableFile = "efficiency_table.txt") {
    std::ifstream inFile(tableFile);
    if (!inFile.is_open()) {
        std::cerr << "Warning: Cannot open efficiency table file: " << tableFile << std::endl;
        std::cerr << "  Angular (and integral) spectra will use normalized counts "
                  << "(no efficiency correction).\n";
        return nullptr;
    }

    EfficiencyTable* table = new EfficiencyTable();
    std::string line;

    while (std::getline(inFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        Double_t energy, eff, err;
        iss >> energy >> eff;

        if (!(iss >> err)) err = 0.0;

        if (energy <= 0.0 || eff <= 0.0) {
            std::cerr << "Warning: Skipping invalid data point: E="
                      << energy << " eff=" << eff << std::endl;
            continue;
        }

        table->energies.push_back(energy);
        table->efficiencies.push_back(eff);
        table->errors.push_back(err);
    }
    inFile.close();

    if (table->energies.empty()) {
        std::cerr << "Warning: No valid data points in efficiency table: "
                  << tableFile << std::endl;
        delete table;
        return nullptr;
    }

    // Verify sorted by energy (should already be from AnalyzeEff.C)
    for (size_t i = 1; i < table->energies.size(); i++) {
        if (table->energies[i] <= table->energies[i-1]) {
            std::cerr << "Warning: Efficiency table not sorted by energy. Sorting now.\n";
            std::vector<size_t> idx(table->energies.size());
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
                return table->energies[a] < table->energies[b];
            });
            std::vector<Double_t> sortedE(table->energies.size());
            std::vector<Double_t> sortedEff(table->energies.size());
            std::vector<Double_t> sortedErr(table->energies.size());
            for (size_t j = 0; j < idx.size(); j++) {
                sortedE[j] = table->energies[idx[j]];
                sortedEff[j] = table->efficiencies[idx[j]];
                sortedErr[j] = table->errors[idx[j]];
            }
            table->energies = std::move(sortedE);
            table->efficiencies = std::move(sortedEff);
            table->errors = std::move(sortedErr);
            break;
        }
    }

    std::cout << "Loaded efficiency table (" << table->energies.size()
              << " data points, " << table->energies.front() << " - "
              << table->energies.back() << " keV) from " << tableFile << std::endl;

    return table;
}

//----------------------------------------------------------------------------//
// Find results matching target angles within tolerance
// Returns vector of pointers to matched AngleResults (preserves target order)
//----------------------------------------------------------------------------//
std::vector<AngleResults*> findMatchingResults(
        std::vector<AngleResults>& results,
        const std::vector<Double_t>& targetAngles,
        Double_t tolerance = 2.5) {

    std::vector<AngleResults*> matched;

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

        if (bestMatch && minDiff <= tolerance) {
            matched.push_back(bestMatch);
        }
    }

    return matched;
}

//----------------------------------------------------------------------------//
// Create a divided canvas with automatic layout for N panels
// Returns the new TCanvas (caller owns it)
//----------------------------------------------------------------------------//
TCanvas* createDividedCanvas(const char* name, const char* title,
                              Int_t nPlots, Int_t padW = 600, Int_t padH = 500) {
    Int_t nCols = (nPlots <= 2) ? nPlots : 2;
    Int_t nRows = (nPlots + nCols - 1) / nCols;

    TCanvas* canvas = new TCanvas(name, title, padW * nCols, padH * nRows);
    canvas->Divide(nCols, nRows);
    return canvas;
}


//============================================================================//
//                             CORE ANALYSIS                                  //
//============================================================================//

//----------------------------------------------------------------------------//
// Analyze a single file or group of files at the same angle
// When includeDistributions=true, also produces NeutronEnergy, NeutronTheta,
// and Energy-vs-Theta histograms (used for first angle group only)
//----------------------------------------------------------------------------//
AngleResults analyzeAngleGroup(Double_t angle, Double_t distance,
                               const std::vector<TString>& files,
                               Bool_t includeDistributions = false) {

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

    // Compute TOF statistics
    auto meanTOF = dfCapture.Mean("TOFEnergy");
    auto stddevTOF = dfCapture.StdDev("TOFEnergy");


    //========================================================================//
    // Fine-binned histograms for angular and integral analysis (1 keV bins, 0-150 keV)
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
    // Optional: full neutron distributions (for first angle group only)
    //========================================================================//
    if (includeDistributions) {
        result.histNeutronEnergy = df.Histo1D(
            {"hNeutronEnergy",
             "Generated Neutron Energy Distribution;Neutron Energy [keV];Counts",
             100, 0, 2200}, "NeutronEnergy");

        result.histNeutronTheta = df.Histo1D(
            {"hNeutronTheta",
             "Generated Neutron Angular Distribution;Neutron Angle [deg];Counts",
             90, 0, 90}, "NeutronTheta");

        result.histEnergyVsTheta = df.Histo2D(
            {"hEnergyVsTheta",
             "Generated Neutron Energy vs Angle;Neutron Angle [deg];Neutron Energy [keV]",
             90, 0, 90, 100, 0, 2200},
            "NeutronTheta", "NeutronEnergy");
    }


    //========================================================================//
    // Trigger computation
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
    result.histTOF_fine = histTOF_fine;
    result.histGenEnergy_fine = histGenEnergy_fine;
    result.histTOFTime_fine = histTOFTime_fine;
    result.histGenVsTOF = histGenVsTOF;
    result.histEnergyDiff = histEnergyDiff;
    result.histEnergyRatio = histEnergyRatio;

    return result;
}


//============================================================================//
//                            PLOTTING FUNCTIONS                              //
//============================================================================//

//----------------------------------------------------------------------------//
// Helper: compute efficiency- and geometry-corrected d²N/(dE dθ) histogram
//
// Returns a newly-allocated TH1D (caller owns) with bin values:
//   with effTable:    2π·sin(θ) · N_cap / (ε(E) · ΔE · ΔΩ)
//   without effTable: 2π·sin(θ) · N_cap / ΔΩ  (geometric weight only)
//----------------------------------------------------------------------------//
TH1D* computeCorrectedSpectrum(AngleResults& res, EfficiencyTable* effTable) {

    Double_t detRadius  = 2.54;                             // Detector radius (cm)
    Double_t thetaRad   = res.angle * TMath::Pi() / 180.0;
    Double_t sinTheta   = TMath::Sin(thetaRad);
    Double_t d          = res.distance;                     // Detector distance (cm)
    Double_t deltaOmega = TMath::Pi() * detRadius * detRadius / (d * d);    // Solid angle of detector
    Double_t geomFactor = 2.0 * TMath::Pi() * sinTheta;     // azimuthal integration factor

    TH1D* h = (TH1D*)res.histTOF_fine->Clone(
        Form("hCorrTOF_%.1f", res.angle));

    for (Int_t bin = 1; bin <= h->GetNbinsX(); bin++) {
        Double_t binCenter = h->GetBinCenter(bin);
        Double_t deltaE    = h->GetBinWidth(bin);
        Double_t counts    = h->GetBinContent(bin);

        if (effTable) {
            Double_t epsilon = effTable->Eval(binCenter);
            if (epsilon > 1e-6) {
                h->SetBinContent(bin, geomFactor * counts / (epsilon * deltaE * deltaOmega));
                h->SetBinError(bin, (counts > 0) ?
                    geomFactor * TMath::Sqrt(counts) / (epsilon * deltaE * deltaOmega) : 0);
            } else {
                h->SetBinContent(bin, 0);
                h->SetBinError(bin, 0);
            }
        } else {
            // No efficiency table: apply geometric factor only for correct relative weighting
            h->SetBinContent(bin, geomFactor * counts / deltaOmega);
            h->SetBinError(bin, (counts > 0) ?
                geomFactor * TMath::Sqrt(counts) / deltaOmega : 0);
        }
    }
    return h;
}

//----------------------------------------------------------------------------//
// Angular and integral spectra
//
// Computes d²N/(dE dθ) once per angle via computeCorrectedSpectrum, then:
//   - Saves a separate normalized plot for each target angle found in the data
//   - Accumulates weighted sum to form the angle-integrated dN/dE spectrum
//
// targetAngles: discrete angles to plot individually (default 10,30,50,60 deg)
//   Any target angle absent from the data is silently skipped.
//----------------------------------------------------------------------------//
void plotAngularIntegralSpectra(std::vector<AngleResults>& results,
                         const PrintFn& printBoth,
                         EfficiencyTable* effTable = nullptr,
                         const std::vector<Double_t>& targetAngles = {10.0, 30.0, 50.0, 60.0}) {

    if (results.empty()) {
        std::cerr << "Warning: No results for angular and integral spectra.\n";
        return;
    }

    // Sorted angles and delta-theta map
    std::vector<Double_t> sortedAngles;
    sortedAngles.reserve(results.size());
    for (const auto& res : results) sortedAngles.push_back(res.angle);
    std::sort(sortedAngles.begin(), sortedAngles.end());

    Int_t nAngles    = (Int_t)sortedAngles.size();
    Double_t degToRad = TMath::Pi() / 180.0;

    // Centered-difference trapezoidal assignment; half-step at boundaries
    std::map<Double_t, Double_t> dThetaMap;
    for (Int_t i = 0; i < nAngles; i++) {
        Double_t dThetaDeg;
        if (nAngles == 1) {
            dThetaDeg = 1.0;
        } else if (i == 0) {
            dThetaDeg = (sortedAngles[1] - sortedAngles[0]) / 2.0;
        } else if (i == nAngles - 1) {
            dThetaDeg = (sortedAngles[nAngles-1] - sortedAngles[nAngles-2]) / 2.0;
        } else {
            dThetaDeg = (sortedAngles[i+1] - sortedAngles[i-1]) / 2.0;
        }
        dThetaMap[sortedAngles[i]] = dThetaDeg * degToRad;
    }

    // Identify which target angles are present in the data
    auto matchedResults = findMatchingResults(results, targetAngles);
    std::set<Double_t> matchedAngleSet;
    for (AngleResults* r : matchedResults) matchedAngleSet.insert(r->angle);

    // Accumulation histograms for integral spectrum
    TH1D* hWeightedTOF = new TH1D("hWeightedTOF_integral",
        "Angle-Integrated TOF Spectrum", 150, 0, 150);
    TH1D* hWeightedGen = new TH1D("hWeightedGen_integral",
        "Angle-Integrated Generated Spectrum", 150, 0, 150);

    TString yAxisLabel = effTable ? "Normalized d^{2}N/(dE d#theta) [arb. units]" : "Normalized Counts / keV";

    printBoth("\nGeometric Weights Applied (factor = 2*pi*sin(theta)*dTheta/deltaOmega):\n");

    Double_t detRadius = 2.54;  // cm (Constants.hh: kDetectorRadius = 5.08 cm / 2)

    //========================================================================//
    // Per-angle correction, plotting, and accumulation
    //========================================================================//
    std::vector<Double_t> plottedAngles;
    for (auto& res : results) {
        Double_t d          = res.distance;
        Double_t thetaRad   = res.angle * degToRad;
        Double_t sinTheta   = TMath::Sin(thetaRad);
        Double_t deltaOmega = TMath::Pi() * detRadius * detRadius / (d * d);
        Double_t dTheta     = dThetaMap[res.angle];  // radians
        Double_t geomFull   = 2.0 * TMath::Pi() * sinTheta / deltaOmega;  // for gen weighting

        std::ostringstream oss;
        oss << "  Angle " << std::setw(6) << std::fixed << std::setprecision(1)
            << res.angle << " deg @ " << std::setprecision(1) << d
            << " cm: dTheta=" << std::setprecision(4) << (dTheta / degToRad)
            << " deg, geomFactor=" << std::setprecision(6) << (geomFull * dTheta) << "\n";
        printBoth(oss.str());

        // Corrected TOF histogram
        TH1D* hCorrTOF = computeCorrectedSpectrum(res, effTable);

        // Generated histogram with the same geometric weight (no efficiency division)
        TH1D* hCorrGen = (TH1D*)res.histGenEnergy_fine->Clone(
            Form("hCorrGen_%.1f", res.angle));
        hCorrGen->Scale(geomFull);  // apply 2πsin(θ)/ΔΩ

        // Accumulate into integral spectrum with weight = dTheta
        hWeightedTOF->Add(hCorrTOF, dTheta);
        hWeightedGen->Add(hCorrGen, dTheta);

        // Per-angle plot for target angles present in data
        if (matchedAngleSet.count(res.angle)) {
            TH1D* hTOFPlot = (TH1D*)hCorrTOF->Clone(
                Form("hTOFPlot_%.1f", res.angle));
            TH1D* hGenPlot = (TH1D*)hCorrGen->Clone(
                Form("hGenPlot_%.1f", res.angle));

            // Normalize both to unit integral independently (pure shape comparison)
            Double_t intTOF = hTOFPlot->Integral();
            Double_t intGen = hGenPlot->Integral();
            if (intTOF > 0) hTOFPlot->Scale(1.0 / intTOF);
            if (intGen > 0) hGenPlot->Scale(1.0 / intGen);

            TCanvas* cAngle = new TCanvas(
                Form("cAngular_%.1fdeg", res.angle),
                Form("Angular Spectrum #theta = %.1f#circ", res.angle),
                800, 600);
            
            // Style settings
            gStyle->SetTitleOffset(0.4);
            gPad->SetGrid(0, 0);
            gPad->SetLeftMargin(0.14);
            gPad->SetRightMargin(0.05);
            gPad->SetBottomMargin(0.12);

            // SimLiT in blue with fill
            hGenPlot->SetFillColor(kBlue);
            hGenPlot->SetFillStyle(3004);
            hGenPlot->SetLineColor(kBlue);
            hGenPlot->SetLineWidth(2);

            // TOF in red with no fill
            hTOFPlot->SetLineColor(kRed);
            hTOFPlot->SetLineWidth(2);
            hTOFPlot->SetFillStyle(0);
            
            // Axis titles and style
            hGenPlot->SetTitle(Form("#theta = %.1f#circ;Neutron Energy [keV];%s",
                                    res.angle, yAxisLabel.Data()));
            hGenPlot->SetStats(0);
            hGenPlot->GetXaxis()->SetTitleOffset(1.55);
            hGenPlot->GetYaxis()->SetTitleOffset(1.55);
            hGenPlot->GetXaxis()->SetLabelSize(0.04);
            hGenPlot->GetYaxis()->SetLabelSize(0.04);
            hGenPlot->GetXaxis()->SetTitleSize(0.04);
            hGenPlot->GetYaxis()->SetTitleSize(0.04);
            hGenPlot->GetXaxis()->CenterTitle(true);
            hGenPlot->GetYaxis()->CenterTitle(true);

            Double_t maxY = TMath::Max(hGenPlot->GetMaximum(), hTOFPlot->GetMaximum());
            hGenPlot->SetMaximum(maxY * 1.2);
            hGenPlot->SetMinimum(0);

            hGenPlot->Draw("HIST");
            hTOFPlot->Draw("HIST SAME");

            TLegend* leg = new TLegend(0.7, 0.72, 0.93, 0.88);
            leg->AddEntry(hGenPlot, "SimLiT", "f");
            leg->AddEntry(hTOFPlot, "Detected", "l");
            leg->SetBorderSize(0);
            leg->Draw();

            cAngle->Update();
            TString outFile = Form("tof_angular_spectrum_%.1fdeg.png", res.angle);
            cAngle->SaveAs(outFile.Data());
            plottedAngles.push_back(res.angle);

            delete hTOFPlot;
            delete hGenPlot;
        }

        delete hCorrTOF;
        delete hCorrGen;
    }


    //========================================================================//
    // Total integral spectrum plot
    //========================================================================//
    TCanvas* cIntegral = new TCanvas("cTotal_integral",
        "Total Integral Spectrum", 800, 600);
    gPad->SetGrid(0, 0);
    gPad->SetLeftMargin(0.14);
    gPad->SetRightMargin(0.05);
    gPad->SetTopMargin(0.05);
    gPad->SetBottomMargin(0.12);

    TH1D* hGenNorm = (TH1D*)hWeightedGen->Clone("hGen_norm");
    TH1D* hTOFNorm = (TH1D*)hWeightedTOF->Clone("hTOF_norm");

    Double_t intGen = hGenNorm->Integral();
    Double_t intTOF = hTOFNorm->Integral();
    if (intGen > 0) hGenNorm->Scale(1.0 / intGen);
    if (intTOF > 0) hTOFNorm->Scale(1.0 / intTOF);

    // SimLiT in blue with fill
    hGenNorm->SetFillColor(kBlue);
    hGenNorm->SetFillStyle(3004);
    hGenNorm->SetLineColor(kBlue);
    hGenNorm->SetLineWidth(2);

    // TOF in red with no fill
    hTOFNorm->SetLineColor(kRed);
    hTOFNorm->SetLineWidth(2);
    hTOFNorm->SetFillStyle(0);

    // Axis titles and style
    hGenNorm->SetTitle(";Neutron Energy [keV];Normalized dN/dE [arb. units]");
    hGenNorm->SetStats(0);
    hGenNorm->GetXaxis()->SetTitleOffset(1.4);
    hGenNorm->GetYaxis()->SetTitleOffset(1.55);
    hGenNorm->GetXaxis()->SetLabelSize(0.04);
    hGenNorm->GetYaxis()->SetLabelSize(0.04);
    hGenNorm->GetXaxis()->SetTitleSize(0.04);
    hGenNorm->GetYaxis()->SetTitleSize(0.04);
    hGenNorm->GetXaxis()->CenterTitle(true);
    hGenNorm->GetYaxis()->CenterTitle(true);

    Double_t maxNorm = TMath::Max(hGenNorm->GetMaximum(), hTOFNorm->GetMaximum());
    hGenNorm->SetMaximum(maxNorm * 1.2);
    hGenNorm->SetMinimum(0);

    hGenNorm->Draw("HIST");
    hTOFNorm->Draw("HIST SAME");

    TLegend* legNorm = new TLegend(0.65, 0.75, 0.88, 0.88);
    legNorm->AddEntry(hGenNorm, "SimLiT", "f");
    legNorm->AddEntry(hTOFNorm, "Detected", "l");
    legNorm->SetBorderSize(0);
    legNorm->Draw();

    cIntegral->Update();
    cIntegral->SaveAs("tof_total_integral_spectrum.png");

    delete hWeightedTOF;
    delete hWeightedGen;

    // Print summary
    printBoth("\nAngular spectra saved for angles: ");
    for (size_t i = 0; i < plottedAngles.size(); i++) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << plottedAngles[i];
        if (i < plottedAngles.size() - 1) oss << ", ";
        printBoth(oss.str());
    }
    printBoth(" degrees\n");
}

//----------------------------------------------------------------------------//
// Diagnostic plots of raw time-of-flight spectra at selected angles
// Identify potential issues with energy calculation from TOF
//----------------------------------------------------------------------------//
void plotTOFTimeSpectra(std::vector<AngleResults>& results,
                        const std::vector<Double_t>& targetAngles = {10.0, 30.0, 50.0, 60.0}) {

    auto matchedResults = findMatchingResults(results, targetAngles);

    if (matchedResults.empty()) {
        std::cerr << "Warning: No matching angles found for TOF time spectra plot.\n";
        return;
    }

    TCanvas* cTOFTime = createDividedCanvas("cTOFTime_diagnostic",
        "TOF Time Spectra (Diagnostic)", matchedResults.size());

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
    for (size_t i = 0; i < matchedResults.size(); i++) {
        std::cout << matchedResults[i]->angle;
        if (i < matchedResults.size() - 1) std::cout << ", ";
    }
    std::cout << " degrees\n";
}

//----------------------------------------------------------------------------//
// Energy Calculation Diagnostic Plots
// Creates scatter plots and histograms to verify TOF energy calculation
// - 2D scatter: Generated vs TOF energy (should lie on y=x line)
// - Energy difference histogram (should be centered at 0)
// - Energy ratio histogram (should be centered at 1.0)
//----------------------------------------------------------------------------//
void plotEnergyDiagnostics(std::vector<AngleResults>& results, const PrintFn& printBoth) {

    if (results.empty()) {
        std::cerr << "Warning: No results for energy diagnostics.\n";
        return;
    }

    printBoth("\n=== Energy Calculation Diagnostics ===\n\n");

    std::vector<Double_t> targetAngles = {10.0, 30.0, 60.0};
    auto selectedResults = findMatchingResults(results, targetAngles);

    if (selectedResults.empty()) {
        printBoth("Warning: No matching angles found for energy diagnostics.\n");
        return;
    }


    //========================================================================//
    // Generated vs TOF Energy Scatter Plots
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
    // Energy Difference Histograms (Gen - TOF)
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
    // Energy Ratio Histograms (TOF / Gen)
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
}


//============================================================================//
//                            MAIN ANALYSIS DRIVER                            //
//============================================================================//

//----------------------------------------------------------------------------//
// Main analysis function
//----------------------------------------------------------------------------//
void analyzeTOF(const char* directory = ".",
                const std::vector<Double_t>& targetAngles = {10.0, 30.0, 50.0, 60.0},
                bool includeDiagnostic = false,
                int nThreads = 0) {

    const char* pattern = "nTOF_";  // filename prefix to identify ROOT files

    // Enable multi-threading
    if (nThreads <= 0) nThreads = std::thread::hardware_concurrency();
    ROOT::EnableImplicitMT(nThreads);    

    // Open output file for saving results
    std::ofstream outFile("analysis_results.txt");
    if (!outFile.is_open()) {
        std::cerr << "Warning: Could not open analysis_results.txt for writing. Results will only print to console.\n";
    }

    // Single printBoth lambda, passed to all functions that need it
    PrintFn printBoth = [&outFile](const std::string& msg) {
        std::cout << msg;
        if (outFile.is_open()) outFile << msg;
    };

    printBoth("\n");
    printBoth("============================================================\n");
    printBoth("    Neutron TOF Data Analysis (RDataFrame)\n");
    printBoth("============================================================\n");
    printBoth("Directory:  " + std::string(directory) + "\n");
    printBoth("Pattern:    " + std::string(pattern) + "*.root\n");
    printBoth("Threads:    " + std::to_string(nThreads) + "\n");
    printBoth("============================================================\n\n");


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
    Int_t firstGroupIdx = -1;

    printBoth("Analyzing " + std::to_string(filesByAngle.size()) + " angle groups...\n");

    Bool_t isFirst = true;
    for (const auto& [angle, angleFiles] : filesByAngle) {
        Double_t distance = distanceByAngle[angle];
        AngleResults res = analyzeAngleGroup(angle, distance, angleFiles, isFirst);

        if (isFirst) {
            firstGroupIdx = results.size();
            isFirst = false;
        }

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
    // Build combined TOF spectrum from per-angle histograms (no extra I/O)
    //========================================================================//
    TH1D* hCombinedTOF = new TH1D("hCombinedTOF",
        "TOF Energy Spectrum (Captured Neutrons);TOF Energy [keV];Counts",
        150, 0, 150);

    ULong64_t totalCapturesAll = 0;
    ULong64_t totalEventsAll = 0;
    ULong64_t totalEntriesAll = 0;

    for (auto& res : results) {
        hCombinedTOF->Add(res.histTOF_fine.GetPtr());
        totalCapturesAll += res.captures;
        totalEventsAll += res.totalEvents;
        totalEntriesAll += res.entries;
    }


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

    for (const auto& res : results) {
        tableStream << std::setw(10) << std::setprecision(3) << res.angle
                    << std::setw(10) << std::setprecision(3) << res.distance
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
    }

    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    tableStream << std::setw(10) << "TOTAL"
                << std::setw(10) << ""
                << std::setw(12) << totalEventsAll
                << std::setw(12) << totalEntriesAll
                << std::setw(12) << totalCapturesAll
                << std::setw(11) << std::setprecision(3) << 100.0 * totalCapturesAll / totalEntriesAll
                << "\n";
    tableStream << "------------------------------------------------------------------------------------------------------------------------------------------------------------\n\n";

    printBoth(tableStream.str());


    //========================================================================//
    // Create standard plots
    //========================================================================//
    gStyle->SetOptStat(0);

    // Prepare data arrays for TGraphErrors
    Int_t nPoints = results.size();
    std::vector<Double_t> angles(nPoints);
    std::vector<Double_t> captures(nPoints);
    std::vector<Double_t> angleErrors(nPoints, 0.0);

    std::vector<Double_t> meanTOFEnergies(nPoints);
    std::vector<Double_t> tofEnergyErrors(nPoints);

    // Generated energy arrays for comparison plot
    std::vector<Double_t> meanGenEnergies(nPoints);
    std::vector<Double_t> genEnergyErrors(nPoints);

    for (Int_t i = 0; i < nPoints; i++) {
        angles[i] = results[i].angle;
        captures[i] = results[i].captures;

        meanTOFEnergies[i] = results[i].meanTOFEnergy;
        tofEnergyErrors[i] = results[i].stderrTOFEnergy;

        meanGenEnergies[i] = results[i].meanGenEnergy;
        genEnergyErrors[i] = results[i].stderrGenEnergy;
    }


    //========================================================================//
    // Energy vs Angle (TOF and Generated comparison)
    //========================================================================//
    Double_t minAngle = *std::min_element(angles.begin(), angles.end());
    Double_t maxAngle = *std::max_element(angles.begin(), angles.end());
    TCanvas* c1 = new TCanvas("c1_tofenergy", "Energy vs Detector Angle", 800, 600);
    c1->SetGrid();

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

    c1->Update();
    c1->SaveAs("tof_energy_vs_angle.png");


    //========================================================================//
    // Histogram of captures
    //========================================================================//
    Double_t binWidth = (nPoints > 1) ? (angles[1] - angles[0]) : 10.0;
    Int_t nBins = (Int_t)((maxAngle - minAngle) / TMath::Abs(binWidth)) + 1;

    TCanvas* c2 = new TCanvas("c2_histogram", "Captures Histogram", 800, 600);
    c2->SetGrid();

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

    c2->Update();
    c2->SaveAs("captures_histogram.png");


    //========================================================================//
    // TOF Energy Spectrum (from summed per-angle histograms)
    //========================================================================//
    TCanvas* c3 = new TCanvas("c3_tof_spectrum", "TOF Energy Spectrum", 800, 600);
    c3->SetGrid();

    TH1D* hTOFSpec = (TH1D*)hCombinedTOF->Clone("hTOFSpec_draw");
    hTOFSpec->SetFillColor(kGreen+2);
    hTOFSpec->SetFillStyle(3004);
    hTOFSpec->SetLineColor(kGreen+2);
    hTOFSpec->SetLineWidth(2);
    hTOFSpec->Draw("HIST");
    c3->Update();
    c3->SaveAs("tof_energy_spectrum.png");

    std::ostringstream tofStatsOutput;
    tofStatsOutput << "TOF Energy Spectrum Statistics:\n"
                   << "  Total captured neutrons: " << totalCapturesAll << "\n"
                   << "  Mean TOF Energy: " << hTOFSpec->GetMean() << " keV\n"
                   << "  Sample Std Dev: " << hTOFSpec->GetStdDev() << " keV\n\n";
    printBoth(tofStatsOutput.str());


    //========================================================================//
    // Generated Neutron Distributions (from first angle group)
    //========================================================================//
    // Find the result that has distribution histograms
    AngleResults* distResult = nullptr;
    for (auto& res : results) {
        if (res.histNeutronEnergy.GetPtr()) {
            distResult = &res;
            break;
        }
    }

    ULong64_t nTotalFirstGroup = distResult ? distResult->totalEvents : 0;

    if (distResult) {
        // Generated Neutron Energy Distribution
        TCanvas* c4 = new TCanvas("c4_neutron_energy", "Generated Neutron Energy Distribution", 800, 600);
        c4->SetGrid();

        TH1D* hNeutronE = (TH1D*)distResult->histNeutronEnergy->Clone("hNeutronE_draw");
        hNeutronE->SetFillColor(kBlue);
        hNeutronE->SetFillStyle(3004);
        hNeutronE->SetLineColor(kBlue);
        hNeutronE->SetLineWidth(2);
        hNeutronE->Draw("HIST");
        c4->Update();

        std::ostringstream neutronEStats;
        neutronEStats << "Generated Neutron Energy Statistics (First Angle Group):\n"
                      << "  Total events: " << nTotalFirstGroup << "\n"
                      << "  Mean Energy: " << hNeutronE->GetMean() << " keV\n"
                      << "  Sample Std Dev: " << hNeutronE->GetStdDev() << " keV\n\n";
        printBoth(neutronEStats.str());

        c4->SaveAs("neutron_energy_distribution.png");

        // Generated Neutron Angular Distribution
        TCanvas* c5 = new TCanvas("c5_neutron_theta", "Generated Neutron Angular Distribution", 800, 600);
        c5->SetGrid();

        TH1D* hNeutronT = (TH1D*)distResult->histNeutronTheta->Clone("hNeutronT_draw");
        hNeutronT->SetFillColor(kRed);
        hNeutronT->SetFillStyle(3004);
        hNeutronT->SetLineColor(kRed);
        hNeutronT->SetLineWidth(2);
        hNeutronT->Draw("HIST");
        c5->Update();

        std::ostringstream neutronTStats;
        neutronTStats << "Generated Neutron Angular Statistics (First Angle Group):\n"
                      << "  Total events: " << nTotalFirstGroup << "\n"
                      << "  Mean Angle: " << hNeutronT->GetMean() << " deg\n"
                      << "  Sample Std Dev: " << hNeutronT->GetRMS() << " deg\n\n";
        printBoth(neutronTStats.str());

        c5->SaveAs("neutron_theta_distribution.png");

        // Neutron Energy vs Angle (2D histogram replaces sampled scatterplot)
        TCanvas* c6 = new TCanvas("c6_energy_vs_theta", "Neutron Energy vs Angle", 800, 600);
        c6->SetGrid();

        TH2D* hEvsT = (TH2D*)distResult->histEnergyVsTheta->Clone("hEvsT_draw");
        hEvsT->SetStats(0);
        hEvsT->Draw("COLZ");
        c6->Update();

        std::ostringstream scatterStats;
        scatterStats << "Energy vs Angle 2D Histogram (First Angle Group):\n"
                     << "  Total events: " << nTotalFirstGroup << "\n\n";
        printBoth(scatterStats.str());

        c6->SaveAs("neutron_energy_vs_theta.png");
    }
    

    //========================================================================//
    // Angular and integral neutron capture energy analysis plots
    //========================================================================//
    printBoth("\n--- Angular and Integral Neutron Capture Energy Analysis ---\n");

    // Load efficiency table if available (generated by AnalyzeEff.C)
    TString effTablePath = TString(directory) + "/efficiency_table.txt";
    EfficiencyTable* effTable = loadEfficiencyTable(effTablePath.Data());

    // Plot angular and integral spectra:
    //   - Per-angle plots (normalized shape comparison) for target angles present in data
    //   - Angle-integrated dN/dE spectrum from all angles
    plotAngularIntegralSpectra(results, printBoth, effTable, targetAngles);

    if (effTable) {
        printBoth("  Angular and integral spectra: efficiency + geometry corrected d^2N/(dE dtheta), normalized shape comparison\n");
        printBoth("  Efficiency table loaded from: " + std::string(effTablePath.Data()) + "\n");
    } else {
        printBoth("  Angular and integral spectra: geometry-weighted, normalized shape comparison (no efficiency correction)\n");
    }


    //========================================================================//
    // Diagnostic plots
    //========================================================================//
    if (includeDiagnostic) {
        // Plot TOF time and energy spectra for diagnostic comparison
        plotTOFTimeSpectra(results);
        plotEnergyDiagnostics(results, printBoth);

        printBoth("Created diagnostic plots of TOF time and energy spectra to identify potential issues.\n");
    }

    printBoth("\n");
    printBoth("============================================================\n");
    printBoth("Analysis complete!\n");
    printBoth("Results saved to: analysis_results.txt\n");
    printBoth("============================================================\n");

    // Cleanup
    if (effTable) delete effTable;
    delete hCombinedTOF;

    // Close output file
    if (outFile.is_open()) {
        outFile.close();
    }
}
