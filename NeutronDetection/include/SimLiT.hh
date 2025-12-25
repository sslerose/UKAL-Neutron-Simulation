// #ifndef SimLiT_h
// #define SimLiT_h 1

// const int RESOLUTION = 1000; 

// class SimLiT
// {

// public:

//   SimLiT(double BeamEnergy, double BeamSigma);  //constructor.
//   ~SimLiT();                                    //destructor.

//   void GenerateNeutron(double &energy, double &theta); //generates one neutron.
  
//   // set-get functions
  
//   void	SetBeamEnergy(double energy);
//   double GetBeamEnergy();

//   void SetBeamSigma(double sigma);
//   double GetBeamSigma();

//   void SetBeamRadius(double Radius);
//   double GetBeamRadius();
  
//   void GetNeutronVelocity(double (&V)[3]);
//   void GetNeutronPosition(double (&r)[2]);

//   bool GetNormalOrStar(); // to know what reaction it was : normal -> 7Li(p,n)7Be = false         star -> 7Li(p,n)7Be* = true

//   void GetInnerParameters(double MinEnergy,double MaxEnergy,double EnergyResolution);
  
//   //counter of proton used by source.
//   void	SetProtonCounter(double counter);
//   double GetProtonCounter();

//   void SetComposition(int composition);
//   int GetComposition();
  
//   void SetTargetThickness(double thickness);
//   double GetTargetThickness();
  
//   void SetSeed(int seed);

//   double GetCDFmax(){return _CDF_max;};
//   double GetDistribution(double Ep, double Theta, int reaction);
//   double PrintCDFvectors();
  
// public:

//   static const int Li;
//   static const int Li3N;
//   static const int Li2O;
//   static const int LiF;
//   static const int LiOH;
//   static const int LiH;
//   static const int LiLi;
  
//   static const int LOW;
//   static const int HIGH;

//   static const double um;   
//   static const double mm;

// private:

//   double GetCDF(double Ep, int vec, int reaction);
//   double GetGauss(double mu,double sigma);
//   void GenerateProton(double &Ep);
//   void ConstructDeltaEVector();
//   void ConstructCDFVectors();
//   double CalculateEnergyLoss(double);
//   bool ProtonReact(double Ep);
//   double CalculateDeltaECm();
//   void GenerateNeutronVelocity(double theta);
//   void DetermineReactionParameters(double Ep);
//   void TransformCMToLab();
//   void ClaculateNeutronEnergy();
//   void CalculateNeutronTheta();
//   void Find_Parameters();
//   double getStoppingPower(double Ep);
//   double calculateDeltaE(double Ep);
//   double getDeltaE(double Ep);
//   double getDeltaEReversed(double Ep, double eps);
  
// private:
//   static const double pi;
//   static const double Eth;
//   static const double Eth_Star;
//   static const double amu; 
//   static const double Mp;
//   static const double Mn;      
//   static const double MLi;
//   static const double MBe;
//   static const double Mcm;  //the reduced mass;  
//   static const double Avog;
//   static const double M_A_7Li;
//   static const double Li7Density[7];
//   static const double StoppingPower[7][4];
//   static const double Li_Be[2121][6];
//   static const double Li_Be_Star[163][6];
//   static const double C;
//   static const double mb; 
//   static const double speedOfLight;
  

//   double _BeamEnergy;
//   double _BeamSigma;  
//   double _BeamRadius;
//   double _Emax;
//   double _Emin;
//   bool	 _NormalOrStar;
//   double _r[2]; // x-y coordinates of the proton (mm)
//   double _Vlab[3],_Vcm[3]; // neutron velocity vectors at lab and cm systems.
//   double _neutronEnergy,_neutronTheta;
//   double _ReactionEnergy; // the energy of the p-n reaction
//   double _CDF_max;
//   double _CDF_vectors[2][8*RESOLUTION+1][3];
//   double deltaEVector[8*RESOLUTION+1];
//   double _deltaECm;
//   double _ProtonCounter;
//   int	 _composition;
//   double _TargetThickness;
//   double _A;
//   double _Li_Zoo_max_energy;
//   double _energy_bin;
//   double _energy_bin_star;


// };

// #endif 

//
// ********************************************************************
// * SimLiT - Simulation of Li Target neutron source                  *
// *                                                                  *
// * Calculates neutron yield from 7Li(p,n)7Be and 7Li(p,n)7Be*       *
// * reactions up to 3.5 MeV (threshold of breakup reaction)          *
// *                                                                  *
// * Original author: Moshe Friedman (December 2018)                  *
// * Modified for Geant4 integration: Thread-safe random numbers      *
// ********************************************************************
//
// I/O units:
//   Length: cm
//   Velocity: cm/s
//   Energy: keV
//   Angle: radians
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#ifndef SimLiT_h
#define SimLiT_h 1

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

const int RESOLUTION = 1000; 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

/// SimLiT - Li(p,n) neutron source simulator
///
/// Generates neutrons from 7Li(p,n)7Be reactions with realistic
/// energy and angular distributions based on evaluated cross sections.
/// 
/// Key features:
/// - Supports both ground state 7Li(p,n)7Be and excited state 7Li(p,n)7Be*
/// - Accounts for proton energy loss in target
/// - Provides energy and angular distributions
/// - Thread-safe random number generation via Geant4's G4UniformRand()

class SimLiT
{

public:

  SimLiT(double BeamEnergy, double BeamSigma);  // Constructor: beam energy and sigma in keV
  ~SimLiT();                                    // Destructor

  //========================================================================//
  // Main neutron generation method
  // Returns energy (keV) and theta (radians) by reference
  //========================================================================//
  void GenerateNeutron(double &energy, double &theta);
  
  //========================================================================//
  // Beam parameter setters and getters
  //========================================================================//
  void   SetBeamEnergy(double energy);   // keV
  double GetBeamEnergy();
  
  void   SetBeamSigma(double sigma);     // keV
  double GetBeamSigma();
  
  void   SetBeamRadius(double Radius);   // cm
  double GetBeamRadius();
  
  //========================================================================//
  // Neutron kinematics access
  //========================================================================//
  void GetNeutronVelocity(double (&V)[3]);   // cm/s
  void GetNeutronPosition(double (&r)[2]);   // cm (x,y position)
  
  // Returns reaction type: false = 7Li(p,n)7Be, true = 7Li(p,n)7Be*
  bool GetNormalOrStar();

  void GetInnerParameters(double MinEnergy, double MaxEnergy, double EnergyResolution);
  
  //========================================================================//
  // Proton counter (tracks number of protons simulated)
  //========================================================================//
  void   SetProtonCounter(double counter);
  double GetProtonCounter();
  
  //========================================================================//
  // Target composition and thickness
  //========================================================================//
  void SetComposition(int composition);
  int  GetComposition();
  
  void   SetTargetThickness(double thickness);  // cm
  double GetTargetThickness();
  
  //========================================================================//
  // Distribution access methods
  //========================================================================//
  double GetCDFmax() { return _CDF_max; }
  double GetDistribution(double Ep, double Theta, int reaction);
  double PrintCDFvectors();
  
public:

  //========================================================================//
  // Target composition indices
  //========================================================================//
  static const int Li;      // Pure lithium
  static const int Li3N;    // Lithium nitride
  static const int Li2O;    // Lithium oxide
  static const int LiF;     // Lithium fluoride (default)
  static const int LiOH;    // Lithium hydroxide
  static const int LiH;     // Lithium hydride
  static const int LiLi;    // Lithium-lithium compound
  
  static const int LOW;
  static const int HIGH;

  //========================================================================//
  // Unit conversion constants
  //========================================================================//
  static const double um;   // micrometer in cm
  static const double mm;   // millimeter in cm

private:

  //========================================================================//
  // Internal calculation methods
  //========================================================================//
  double GetCDF(double Ep, int vec, int reaction);
  double GetGauss(double mu, double sigma);
  void   GenerateProton(double &Ep);
  void   ConstructDeltaEVector();
  void   ConstructCDFVectors();
  double CalculateEnergyLoss(double);
  bool   ProtonReact(double Ep);
  double CalculateDeltaECm();
  void   GenerateNeutronVelocity(double theta);
  void   DetermineReactionParameters(double Ep);
  void   TransformCMToLab();
  void   ClaculateNeutronEnergy();
  void   CalculateNeutronTheta();
  void   Find_Parameters();
  double getStoppingPower(double Ep);
  double calculateDeltaE(double Ep);
  double getDeltaE(double Ep);
  double getDeltaEReversed(double Ep, double eps);
  
private:

  //========================================================================//
  // Physical constants (all energies in keV)
  //========================================================================//
  static const double pi;
  static const double Eth;           // Threshold energy for 7Li(p,n)7Be
  static const double Eth_Star;      // Threshold energy for 7Li(p,n)7Be*
  static const double amu;           // Atomic mass unit in keV
  static const double Mp;            // Proton mass
  static const double Mn;            // Neutron mass
  static const double MLi;           // Li-7 mass
  static const double MBe;           // Be-7 mass
  static const double Mcm;           // Reduced mass
  static const double Avog;          // Avogadro's number
  static const double M_A_7Li;       // Li-7 atomic mass in amu
  static const double Li7Density[7]; // Density for each compound
  static const double StoppingPower[7][4];  // Stopping power coefficients
  static const double Li_Be[2121][6];       // Cross section data (ground state)
  static const double Li_Be_Star[163][6];   // Cross section data (excited state)
  static const double C;
  static const double mb;            // millibarn in cm^2
  static const double speedOfLight;  // cm/s

  //========================================================================//
  // Internal state variables
  //========================================================================//
  double _BeamEnergy;           // Proton beam energy (keV)
  double _BeamSigma;            // Beam energy spread (keV)
  double _BeamRadius;           // Beam spot radius (cm)
  double _Emax;                 // Maximum energy in distribution
  double _Emin;                 // Minimum energy in distribution
  bool   _NormalOrStar;         // Reaction type flag
  double _r[2];                 // Proton x-y position (cm)
  double _Vlab[3], _Vcm[3];     // Neutron velocity in lab and CM frames
  double _neutronEnergy;        // Generated neutron energy (keV)
  double _neutronTheta;         // Generated neutron angle (radians)
  double _ReactionEnergy;       // Energy at which reaction occurs
  double _CDF_max;              // Maximum CDF value for normalization
  double _CDF_vectors[2][8*RESOLUTION+1][3];  // CDF lookup tables
  double deltaEVector[8*RESOLUTION+1];        // Energy loss lookup table
  double _deltaECm;             // Energy available in CM frame
  double _ProtonCounter;        // Number of protons simulated
  int    _composition;          // Target composition index
  double _TargetThickness;      // Target thickness (cm)
  double _A;                    // Cross section fitting parameter
  double _Li_Zoo_max_energy;    // Max energy for low-energy formula
  double _energy_bin;           // Energy bin size for ground state
  double _energy_bin_star;      // Energy bin size for excited state
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

#endif 