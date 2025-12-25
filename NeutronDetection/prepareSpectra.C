// #include "TFile.h"
// #include "TTree.h"
// #include "TH1D.h"
// #include "TH2D.h"
// #include "TCanvas.h"
// #include "TLegend.h"
// #include "TF1.h"
// #include "TStyle.h"
// #include <cstdio>

//
// prepareSpectra.C
// ROOT analysis macro for Neutron Detection simulation (Option B TOF)
//
// This macro analyzes the output from the Geant4 neutron detection simulation
// where TOF is measured as the time of neutron capture in the Li-6 glass.
//
// Usage:
//   root -l NeutronDetection.root
//   .L prepareSpectra.C
//   analyzeNeutronDetection()     // Full analysis with plots
//
//   // Or step by step:
//   prepareSpectra("NeutronDetection.root")
//   drawSpectra()
//
// Ntuple structure (from simulation):
//   NeutronEnergy  (Double) - SimLiT neutron energy [keV]
//   NeutronTheta   (Double) - SimLiT neutron angle [deg]
//   TotalEdep      (Double) - Energy deposited [MeV]
//   TOF            (Double) - Time of flight = capture time [ns], -1 if no capture
//   CaptureFlag    (Int)    - 1 if capture, 0 otherwise
//

//============================================================================//
// Global histogram pointers
//============================================================================//

// 1D histograms
TH1D* hNeutronEnergy;      // SimLiT energy distribution
TH1D* hNeutronTheta;       // SimLiT angle distribution
TH1D* hTotalEdep;          // All energy depositions
TH1D* hEdepCapture;        // Energy when capture occurs
TH1D* hTOF;                // Time of flight (capture events)

// 2D correlation histograms
TH2D* hEnergyVsTOF;        // Key plot: neutron energy vs TOF
TH2D* hEnergyVsEdep;       // Energy vs deposited energy
TH2D* hThetaVsTOF;         // Angle vs TOF

// Efficiency histogram
TH1D* hEfficiency;         // Detection efficiency vs neutron energy

//============================================================================//
// Main analysis function
//============================================================================//
void prepareSpectra(const char* filename = "NeutronDetection.root") {
    
    TFile* inFile = TFile::Open(filename, "READ");
    if (!inFile || inFile->IsZombie()) {
        printf("ERROR: Cannot open file: %s\n", filename);
        return;
    }
    printf("Analyzing file: %s\n", filename);
    
    // Get the ntuple
    TTree* data = (TTree*)inFile->Get("DetectorData");
    if (!data) {
        printf("ERROR: Cannot find DetectorData tree\n");
        printf("Available objects in file:\n");
        inFile->ls();
        return;
    }
    
    Long64_t nEvents = data->GetEntries();
    printf("Total events: %lld\n\n", nEvents);
    
    //========================================================================//
    // Set up branch addresses
    //========================================================================//
    Double_t neutronEnergy, neutronTheta, totalEdep, tof;
    Int_t captureFlag;
    
    data->SetBranchAddress("NeutronEnergy", &neutronEnergy);
    data->SetBranchAddress("NeutronTheta", &neutronTheta);
    data->SetBranchAddress("TotalEdep", &totalEdep);
    data->SetBranchAddress("TOF", &tof);
    data->SetBranchAddress("CaptureFlag", &captureFlag);
    
    //========================================================================//
    // Create histograms
    //========================================================================//
    
    // SimLiT source distributions
    hNeutronEnergy = new TH1D("hNeutronEnergy",
        "SimLiT Neutron Energy;Energy [keV];Events",
        200, 0, 200);
    
    hNeutronTheta = new TH1D("hNeutronTheta",
        "SimLiT Neutron Angle;Angle [deg];Events",
        90, 0, 90);
    
    // Detector response
    hTotalEdep = new TH1D("hTotalEdep",
        "Total Energy Deposited;Energy [MeV];Events",
        200, 0, 6);
    
    hEdepCapture = new TH1D("hEdepCapture",
        "Energy Deposited (Capture Events);Energy [MeV];Events",
        200, 0, 6);
    
    // TOF distribution (Option B: capture time)
    hTOF = new TH1D("hTOF",
        "Time of Flight (Capture Time);TOF [ns];Events",
        500, 0, 5000);
    
    // 2D correlations
    hEnergyVsTOF = new TH2D("hEnergyVsTOF",
        "Neutron Energy vs TOF;Neutron Energy [keV];TOF [ns]",
        100, 0, 200,
        250, 0, 5000);
    
    hEnergyVsEdep = new TH2D("hEnergyVsEdep",
        "Neutron Energy vs Energy Deposited;Neutron Energy [keV];Energy [MeV]",
        100, 0, 200,
        60, 0, 6);
    
    hThetaVsTOF = new TH2D("hThetaVsTOF",
        "Neutron Angle vs TOF;Angle [deg];TOF [ns]",
        45, 0, 90,
        250, 0, 5000);
    
    // For efficiency calculation
    TH1D* hEnergyAll = new TH1D("hEnergyAll", "All neutrons", 40, 0, 200);
    TH1D* hEnergyCaptured = new TH1D("hEnergyCaptured", "Captured neutrons", 40, 0, 200);
    
    //========================================================================//
    // Event loop
    //========================================================================//
    Long64_t nCaptures = 0;
    Long64_t nWithEdep = 0;
    
    for (Long64_t i = 0; i < nEvents; i++) {
        data->GetEntry(i);
        
        // Fill source distributions (all events)
        hNeutronEnergy->Fill(neutronEnergy);
        hNeutronTheta->Fill(neutronTheta);
        hEnergyAll->Fill(neutronEnergy);
        
        // Fill energy deposition (if any)
        if (totalEdep > 0) {
            nWithEdep++;
            hTotalEdep->Fill(totalEdep);
            hEnergyVsEdep->Fill(neutronEnergy, totalEdep);
        }
        
        // Fill capture-specific histograms
        if (captureFlag == 1 && tof > 0) {
            nCaptures++;
            hTOF->Fill(tof);
            hEdepCapture->Fill(totalEdep);
            hEnergyVsTOF->Fill(neutronEnergy, tof);
            hThetaVsTOF->Fill(neutronTheta, tof);
            hEnergyCaptured->Fill(neutronEnergy);
        }
        
        // Progress
        if (i % 50000 == 0 && i > 0) {
            printf("  Processed %lld / %lld events (%.1f%%)\n",
                   i, nEvents, 100.0 * i / nEvents);
        }
    }
    
    //========================================================================//
    // Calculate detection efficiency vs energy
    //========================================================================//
    hEfficiency = (TH1D*)hEnergyCaptured->Clone("hEfficiency");
    hEfficiency->SetTitle("Detection Efficiency vs Neutron Energy;Energy [keV];Efficiency");
    hEfficiency->Divide(hEnergyAll);
    
    // Clean up temporary histograms
    delete hEnergyAll;
    delete hEnergyCaptured;
    
    //========================================================================//
    // Print summary
    //========================================================================//
    printf("\n");
    printf("============================================================\n");
    printf("                   Analysis Summary\n");
    printf("============================================================\n");
    printf("Total events:        %lld\n", nEvents);
    printf("Events with Edep>0:  %lld (%.2f%%)\n", 
           nWithEdep, 100.0 * nWithEdep / nEvents);
    printf("Capture events:      %lld (%.2f%%)\n",
           nCaptures, 100.0 * nCaptures / nEvents);
    printf("\n");
    printf("SimLiT Source:\n");
    printf("  Mean energy: %.2f ± %.2f keV\n",
           hNeutronEnergy->GetMean(), hNeutronEnergy->GetRMS());
    printf("  Mean angle:  %.2f ± %.2f deg\n",
           hNeutronTheta->GetMean(), hNeutronTheta->GetRMS());
    printf("\n");
    printf("TOF Measurement (Option B - Capture Time):\n");
    printf("  Mean TOF:    %.1f ± %.1f ns\n",
           hTOF->GetMean(), hTOF->GetRMS());
    printf("  Mean Edep:   %.3f ± %.3f MeV (capture events)\n",
           hEdepCapture->GetMean(), hEdepCapture->GetRMS());
    printf("============================================================\n");
}

//============================================================================//
// Draw histograms on canvases
//============================================================================//
void drawSpectra() {
    
    if (!hNeutronEnergy) {
        printf("ERROR: Run prepareSpectra() first\n");
        return;
    }
    
    gStyle->SetOptStat(1111);
    gStyle->SetOptFit(1);
    
    //========================================================================//
    // Canvas 1: SimLiT Source Distributions
    //========================================================================//
    TCanvas* c1 = new TCanvas("c1_source", "SimLiT Neutron Source", 1000, 500);
    c1->Divide(2, 1);
    
    c1->cd(1);
    hNeutronEnergy->SetLineColor(kBlue);
    hNeutronEnergy->SetLineWidth(2);
    hNeutronEnergy->Draw();
    
    c1->cd(2);
    hNeutronTheta->SetLineColor(kRed);
    hNeutronTheta->SetLineWidth(2);
    hNeutronTheta->Draw();
    
    //========================================================================//
    // Canvas 2: Detector Response
    //========================================================================//
    TCanvas* c2 = new TCanvas("c2_detector", "Detector Response", 1000, 500);
    c2->Divide(2, 1);
    
    c2->cd(1);
    hTotalEdep->SetLineColor(kBlue);
    hTotalEdep->SetLineWidth(2);
    hTotalEdep->Draw();
    hEdepCapture->SetLineColor(kRed);
    hEdepCapture->SetLineWidth(2);
    hEdepCapture->SetFillColor(kRed);
    hEdepCapture->SetFillStyle(3004);
    hEdepCapture->Draw("same");
    
    TLegend* leg1 = new TLegend(0.55, 0.7, 0.88, 0.88);
    leg1->AddEntry(hTotalEdep, "All events", "l");
    leg1->AddEntry(hEdepCapture, "Capture events", "f");
    leg1->Draw();
    
    c2->cd(2);
    hTOF->SetLineColor(kGreen+2);
    hTOF->SetLineWidth(2);
    hTOF->Draw();
    
    //========================================================================//
    // Canvas 3: Key Correlation - Energy vs TOF
    //========================================================================//
    TCanvas* c3 = new TCanvas("c3_EvsTOF", "Energy vs TOF Correlation", 800, 600);
    gPad->SetLogz();
    hEnergyVsTOF->Draw("COLZ");
    
    // Add theoretical curve: TOF = d/v = d * sqrt(m_n / 2E)
    // For d = 51 cm, m_n = 939.565 MeV/c^2
    // TOF [ns] = 51 [cm] * sqrt(939565 [keV] / (2 * E [keV])) / c
    // TOF [ns] = 51 * sqrt(939565 / 2E) / 29.98 = 1.70 * sqrt(939565/2E)
    TF1* fTOF = new TF1("fTOF", "51.0 * sqrt(939565.0 / (2.0 * x)) / 29.979", 5, 200);
    fTOF->SetLineColor(kRed);
    fTOF->SetLineWidth(2);
    fTOF->SetLineStyle(2);
    fTOF->Draw("same");
    
    TLegend* leg3 = new TLegend(0.5, 0.75, 0.88, 0.88);
    leg3->AddEntry(fTOF, "Theoretical: d=51cm", "l");
    leg3->Draw();
    
    //========================================================================//
    // Canvas 4: Detection Efficiency
    //========================================================================//
    TCanvas* c4 = new TCanvas("c4_efficiency", "Detection Efficiency", 800, 600);
    hEfficiency->SetLineColor(kBlue);
    hEfficiency->SetLineWidth(2);
    hEfficiency->SetMarkerStyle(20);
    hEfficiency->SetMarkerColor(kBlue);
    hEfficiency->Draw("E");
    gPad->SetGrid();
    
    //========================================================================//
    // Canvas 5: Other correlations
    //========================================================================//
    TCanvas* c5 = new TCanvas("c5_corr", "Additional Correlations", 1000, 500);
    c5->Divide(2, 1);
    
    c5->cd(1);
    hEnergyVsEdep->Draw("COLZ");
    
    c5->cd(2);
    hThetaVsTOF->Draw("COLZ");
    
    printf("\nCanvases created:\n");
    printf("  c1_source     - SimLiT source distributions\n");
    printf("  c2_detector   - Detector response\n");
    printf("  c3_EvsTOF     - Energy vs TOF (key plot)\n");
    printf("  c4_efficiency - Detection efficiency vs energy\n");
    printf("  c5_corr       - Additional correlations\n");
}

//============================================================================//
// Complete analysis: load, analyze, and draw
//============================================================================//
void analyzeNeutronDetection(const char* filename = "NeutronDetection.root") {
    prepareSpectra(filename);
    drawSpectra();
}
