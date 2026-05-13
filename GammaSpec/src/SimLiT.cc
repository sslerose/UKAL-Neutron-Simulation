/*
SimLiT 

Calculate the neutron yield out of 7Li(p,n)7Be and 7Li(p,n)7Be* reactions up to 3.5 MeV {the thrashold of breakup -> 7Li(p,n3He)4He }

I/O units:
Length: cm.
Velocity: cm/s.
Energy: keV.
Angle: radians.

updated: December 2018.   Author: Moshe Friedman.   Email: moshe.friedman@mail.huji.ac.il

assumptions:
- from 1880 keV up to 1890 the cross section is calculated and assume to be spheric symmetric for 7Li(p,n)7Be
- from 2373 up to 2500 keV the cross section is calculated and assume to be spheric symmetric for 7Li(p,n)7Be*
*/

#include "SimLiT.hh" 
#include "Randomize.hh"

#include <cmath>
#include <cfloat>
#include "SimLiT_data.hh"
#include <iostream>
#include <G4ios.hh>
#include <G4Threading.hh>

//constants
const double SimLiT::pi = 3.14159265;
const double SimLiT::Eth = 1880.36;  //keV
const double SimLiT::Eth_Star = 2373.36;  //keV
const double SimLiT::amu = 931494;   //keV
const double SimLiT::Mp = 1.007825*amu - 511;	//keV
const double SimLiT::Mn = 1.008665*amu;  //keV    
const double SimLiT::MLi = 7.016004*amu - 3*511;	//keV
const double SimLiT::MBe = 7.016929*amu - 4*511;	//keV
const double SimLiT::Mcm = Mp*MLi/(Mp+MLi);  
const double SimLiT::Avog = 6.022e23;	// amu/g
const double SimLiT::M_A_7Li = 7.016; //amu 
const double SimLiT::mb = 1e-27;  // cm^2
const double SimLiT::um = 1e-4;   // cm   
const double SimLiT::mm = 0.1;   // cm  
const double SimLiT::speedOfLight = 29979245800;    //cm/sec

//indeces
const int SimLiT::Li = 0;
const int SimLiT::Li3N = 1;
const int SimLiT::Li2O = 2;
const int SimLiT::LiF = 3;
const int SimLiT::LiOH = 4;
const int SimLiT::LiH = 5;
const int SimLiT::LiLi = 6;
const int SimLiT::LOW = 0;
const int SimLiT::HIGH = 1;

// for common Li compounds, 7Li density in g/cm3                                     
const double SimLiT::Li7Density[7] = {0.499,0.709,0.935,0.659,0.395,0.669,0.4784};

//variable needed for lowest energies fit
const double SimLiT::C = 6;

G4ThreadLocal double debug_Ncalls = 0;
G4ThreadLocal double debug_Noverflow = 0;
G4ThreadLocal double debug_Ntries = 0;


SimLiT::SimLiT(double BeamEnergy, double BeamSigma)
{
  Find_Parameters();

  _BeamEnergy = BeamEnergy;
  _BeamSigma = BeamSigma;
  SetBeamRadius(2*mm);
  SetProtonCounter(0);
  _composition = LiF;
  _TargetThickness = 20*um;

  ConstructDeltaEVector();
  ConstructCDFVectors();  
}

// SimLiT::~SimLiT()
// {
//   printf("\n==================================================\n");
//   printf("DEBUG: overflow rate = %.2f ; tries rate = %.3f\n",debug_Noverflow/debug_Ncalls,debug_Ntries/debug_Ncalls);
//   printf("\n==================================================\n");
  
// }

SimLiT::~SimLiT()
{
  //========================================================================//
  // Print debug statistics on destruction
  //========================================================================//
  if (debug_Ncalls > 0) {
    G4cout << G4endl;
    G4cout << "==================================================" << G4endl;
    G4cout << "SimLiT Statistics:" << G4endl;
    G4cout << "  Overflow rate = " << debug_Noverflow/debug_Ncalls << G4endl;
    G4cout << "  Tries rate    = " << debug_Ntries/debug_Ncalls << G4endl;
    G4cout << "==================================================" << G4endl;
  }
}

void SimLiT::Find_Parameters()
{
  // energy bin
  _energy_bin = Li_Be[1][0]-Li_Be[0][0];
  _energy_bin_star = Li_Be_Star[1][0]-Li_Be_Star[0][0];
  
  // finding the correct A according to first cross section measured point
  _Li_Zoo_max_energy = 1886.7;
  double x,cs,sigma_0,a0;
  double a,b;
  int i;
    
  i=int(floor((_Li_Zoo_max_energy-Li_Be[0][0])/_energy_bin));
  
  // set sigma_0
  a = (Li_Be[i][1]-Li_Be[i-1][1])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
  b = Li_Be[i][1] - a*Li_Be[i][0];                                   //b=y2-a*x2
  sigma_0 = a*_Li_Zoo_max_energy+b;
  // set a0
  a = (Li_Be[i][2]-Li_Be[i-1][2])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
  b = Li_Be[i][2] - a*Li_Be[i][0];                                   //b=y2-a*x2
  a0 = a*_Li_Zoo_max_energy+b;
  
  x = C*sqrt(1-(Eth/_Li_Zoo_max_energy));
  cs = 1/(_Li_Zoo_max_energy/1000)*x/pow((1+x),2); // mb
  
  _A = sigma_0 * a0 / cs;
 
  return;
}

// void SimLiT::GenerateNeutron(double &energy, double &theta)
// {
//   if ((_BeamEnergy+3*_BeamSigma)<Eth)
//   {
//     energy = 0;
//     theta = 0;
//     SetProtonCounter(DBL_MAX);
//   }
//   else
//   {
//     double Ep;
//     GenerateProton(Ep);
//     while (!(ProtonReact(Ep))) GenerateProton(Ep);
//     DetermineReactionParameters(Ep);
//     TransformCMToLab();
//     ClaculateNeutronEnergy();
//     CalculateNeutronTheta();
    
//     energy = _neutronEnergy;
//     theta = _neutronTheta;
//    // printf("DEBUG: dE/dx = %.2f deltaE = %.2f\n",getStoppingPower(Ep),getDeltaE(Ep));

//   }
// }


void SimLiT::GenerateNeutron(double &energy, double &theta)
{
  //========================================================================//
  // Main neutron generation method
  // Returns neutron energy (keV) and angle (radians) by reference
  //========================================================================//
  
  if ((_BeamEnergy + 3*_BeamSigma) < Eth) {
    // Beam energy too low for any reactions
    energy = 0;
    theta = 0;
    SetProtonCounter(DBL_MAX);
  }
  else {
    double Ep;
    GenerateProton(Ep);
    
    // Keep generating protons until one reacts
    while (!(ProtonReact(Ep))) {
      GenerateProton(Ep);
    }
    
    DetermineReactionParameters(Ep);
    TransformCMToLab();
    ClaculateNeutronEnergy();
    CalculateNeutronTheta();
    
    energy = _neutronEnergy;
    theta = _neutronTheta;
  }
}

void SimLiT::SetBeamEnergy(double energy)
{
  _BeamEnergy = energy;
  ConstructDeltaEVector();
  ConstructCDFVectors();    
}

double SimLiT::GetBeamEnergy()
{
  return _BeamEnergy;
}

void SimLiT::SetBeamSigma(double sigma)
{
  _BeamSigma = sigma;
  ConstructDeltaEVector();
  ConstructCDFVectors();    
}

double SimLiT::GetBeamSigma()
{
  return _BeamSigma;
}

void SimLiT::SetBeamRadius(double Radius)
{
  _BeamRadius = Radius;
}

double SimLiT::GetBeamRadius()
{
  return _BeamRadius;
}

void SimLiT::GetNeutronVelocity(double (&V)[3])
{
  V[0] = _Vlab[0]*speedOfLight;
  V[1] = _Vlab[1]*speedOfLight;
  V[2] = _Vlab[2]*speedOfLight;
}

void SimLiT::GetNeutronPosition(double (&r)[2])
{
  r[0] = _r[0];
  r[1] = _r[1];
}

void SimLiT::SetProtonCounter(double counter)
{
  _ProtonCounter = counter;
}

double SimLiT::GetProtonCounter()
{
  return _ProtonCounter;
}

void SimLiT::SetComposition(int composition)
{
  _composition = composition;
  ConstructDeltaEVector();
  ConstructCDFVectors();    
}

int SimLiT::GetComposition()
{
  return _composition;
}

void SimLiT::SetTargetThickness(double thickness)
{
  _TargetThickness = thickness;
  ConstructDeltaEVector();
  ConstructCDFVectors();
}

double SimLiT::GetTargetThickness()
{
  return _TargetThickness;
}

bool SimLiT::GetNormalOrStar()
{
  return _NormalOrStar;
}
       
// void SimLiT::SetSeed(int seed)
// {
//   srand(seed);
// }

double SimLiT::GetGauss(double mu,double sigma)
{

  // Normally distribured random number generator by Box-Muller method.
  // G. E. P. Box and Mervin E. Muller, A Note on the Generation of Random Normal Deviates, 
  // The Annals of Mathematical Statistics (1958), Vol. 29, No. 2 pp. 610–611

  double x,u,v;
  // u = rand()/(RAND_MAX*1.0);
  // v = rand()/(RAND_MAX*1.0);
  u = G4UniformRand();
  v = G4UniformRand();
  x = sqrt(-2.*log(u))*cos(2*pi*v);
  return sigma*x+mu;
}

void SimLiT::GenerateProton(double &Ep)
{                            
  Ep = GetGauss(_BeamEnergy,_BeamSigma);   
  double R,theta;
  R = GetGauss(0,_BeamRadius);
  // theta = pi*rand()/(RAND_MAX*1.0);
  theta = pi * G4UniformRand();
  _r[0] = R*cos(theta); //cm
  _r[1] = R*sin(theta); //cm
  SetProtonCounter(GetProtonCounter() + 1./_CDF_max);
}

//reaction: 0 = all, 1 = 7Li(p,n)7Be, 2 = 7Li(p,n)7Be*
double SimLiT::GetDistribution(double Ep, double Theta, int reaction = 0)
{
  if(Ep>4000)
	Ep=_BeamEnergy;
  if (Ep<Eth)
    return 0;
  else
  {
    double x,dist,CS,CS_Star,dEdx,ro,prob;
    double sigma_0,a0,a1,a2,a3;
    double sigma_0s,a0s,a1s,a2s,a3s;
    double a,b;

    double P0,P1,P2,P3;
    int i=0;
    
    if (Ep<=_Li_Zoo_max_energy)
    {
      x = C*sqrt(1-(Eth/Ep));
      CS = _A/(Ep/1000)*x/pow((1+x),2); // mb
      CS_Star=0;
    }
    else
    {
      i=int(floor((Ep-Li_Be[0][0])/_energy_bin));
      
      // set sigma_0
      a = (Li_Be[i][1]-Li_Be[i-1][1])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
      b = Li_Be[i][1] - a*Li_Be[i][0];                     //b=y2-a*x2
      sigma_0 = a*Ep+b;
      // set a0
      a = (Li_Be[i][2]-Li_Be[i-1][2])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
      b = Li_Be[i][2] - a*Li_Be[i][0];                     //b=y2-a*x2
      a0 = a*Ep+b;
      // set a1
      a = (Li_Be[i][3]-Li_Be[i-1][3])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
      b = Li_Be[i][3] - a*Li_Be[i][0];                     //b=y2-a*x2
      a1 = a*Ep+b;
      // set a2
      a = (Li_Be[i][4]-Li_Be[i-1][4])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
      b = Li_Be[i][4] - a*Li_Be[i][0];                     //b=y2-a*x2
      a2 = a*Ep+b;
      // set a3
      a = (Li_Be[i][5]-Li_Be[i-1][5])/_energy_bin;         //a=(y2-y1)/(x2-x1)	
      b = Li_Be[i][5] - a*Li_Be[i][0];                     //b=y2-a*x2
      a3 = a*Ep+b;
      
      P0 = a0;								//      p0 	1
      P1 = a1*cos(Theta);						//      p1 	x
      P2 = a2*0.5*(3*cos(Theta)*cos(Theta)-1);				//      p2 	0.5 * (3x^2-1)
      P3 = a3*0.5*(5*cos(Theta)*cos(Theta)*cos(Theta)-3*cos(Theta));	//      p3 	0.5 * (5x^3-3x)_ReactionEnergy
      CS = sigma_0*(P0+P1+P2+P3);
      
      if(Ep>Eth_Star)
      {
	i=int(floor((Ep-Li_Be_Star[0][0])/_energy_bin_star));

	// set sigma_0
	a = (Li_Be_Star[i][1]-Li_Be_Star[i-1][1])/_energy_bin_star;
	b = Li_Be_Star[i][1] - a*Li_Be_Star[i][0];
	sigma_0s = a*Ep+b;
	// set a0
	a = (Li_Be_Star[i][2]-Li_Be_Star[i-1][2])/_energy_bin_star;
	b = Li_Be_Star[i][2] - a*Li_Be_Star[i][0];
	a0s = a*Ep+b;
	// set a1
	a = (Li_Be_Star[i][3]-Li_Be_Star[i-1][3])/_energy_bin_star;
	b = Li_Be_Star[i][3] - a*Li_Be_Star[i][0];
	a1s = a*Ep+b;
	// set a2
	a = (Li_Be_Star[i][4]-Li_Be_Star[i-1][4])/_energy_bin_star;
	b = Li_Be_Star[i][4] - a*Li_Be_Star[i][0];
	a2s = a*Ep+b;
	// set a3
	a = (Li_Be_Star[i][5]-Li_Be_Star[i-1][5])/_energy_bin_star;
	b = Li_Be_Star[i][5] - a*Li_Be_Star[i][0];
	a3s = a*Ep+b;
	
	P0 = a0s;
	P1 = a1s*cos(Theta);
	P2 = a2s*0.5*(3*cos(Theta)*cos(Theta)-1);
	P3 = a3s*0.5*(5*cos(Theta)*cos(Theta)*cos(Theta)-3*cos(Theta));
	CS_Star = sigma_0s*(P0+P1+P2+P3);
      }
      else
	CS_Star=0;
    }
    
    dEdx = getStoppingPower(Ep);
    ro = Li7Density[_composition]; // g/cm3
    if (reaction==0)
      dist = (ro*Avog/M_A_7Li)*(CS+CS_Star)/dEdx*mb*um;  // 1/keV/str
    else if (reaction==1)
      dist = (ro*Avog/M_A_7Li)*CS/dEdx*mb*um;  // 1/keV/str
    else if (reaction==2)
      dist = (ro*Avog/M_A_7Li)*CS_Star/dEdx*mb*um;  // 1/keV/str
    else dist=0;
    
    return dist;
  }
}

void SimLiT::ConstructCDFVectors()
{
   double E_eps,energy,energy_loss;
   int j,i;
   for (i=0;i<=(8*RESOLUTION);i++)
     for (j=0;j<3;j++)
       _CDF_vectors[LOW][i][j] = _CDF_vectors[HIGH][i][j] = 0;
   i = -1;
   double CDF[3] = {0,0,0};
      
   E_eps = _BeamSigma/(RESOLUTION*1.0); // keV

   for (j=0;j<3;j++)
     for (energy=Eth;energy<(_BeamEnergy+3*_BeamSigma+0.1);energy=energy+E_eps)
     {
      // printf("\renergy = %.4f",energy);
       CDF[j] = CDF[j] + GetDistribution(energy,pi/2,j)*4*pi*E_eps;
       energy_loss = getDeltaEReversed(energy,E_eps);
     
       i = (int) floor((energy+energy_loss-(_BeamEnergy-5*_BeamSigma))/E_eps);
       if ((i>=0)&&(i<=(8*RESOLUTION)))
         _CDF_vectors[LOW][i][j] = CDF[j];
      
       
       i = (int) floor((energy-(_BeamEnergy-5*_BeamSigma))/E_eps);
       if ((i>=0)&&(i<=(8*RESOLUTION)))
         _CDF_vectors[HIGH][i][j] = CDF[j];
     }

   _Emax = _BeamEnergy + 3*_BeamSigma;
   _Emin = _BeamEnergy - 5*_BeamSigma;
   _CDF_max = GetCDF(_Emax,HIGH,0) - GetCDF(_Emin,LOW,0);

   if (_CDF_max<=0)
      _CDF_max = 1./DBL_MAX;
}

double SimLiT::GetCDF(double Ep, int vec, int reaction = 0)
{
  int e;
  double step;
  step = _BeamSigma/(RESOLUTION*1.0);
  e = (int) floor((Ep-(_BeamEnergy-5*_BeamSigma))/step);
  if (e<0) e = 0;
  else if (e>(8*RESOLUTION)) e = (8*RESOLUTION);

  return _CDF_vectors[vec][e][reaction];   
}

bool SimLiT::ProtonReact(double Ep)
{
  // This function checks if the proton will make p-n reaction.
  // We normalize the probability such that CDF(beamEnergy+3*sigma)-CDF(beamEnergy-5*sigma)=1
  // Proton with higher energy will react with probability of 1, and the normaliztaion makes an error of ~0.001
  
  double prob0,prob1,rand1;
  prob0 = (GetCDF(Ep,HIGH,1)-GetCDF(Ep,LOW,1))/_CDF_max; 
  prob1 = (GetCDF(Ep,HIGH,2)-GetCDF(Ep,LOW,2))/_CDF_max; 
  
  
  // rand1 = rand()/(RAND_MAX*1.0);
  rand1 = G4UniformRand();
  if ((rand1 < prob0)&&(Ep>Eth)) 
  {
    _NormalOrStar = false;
    return true;
  }
  else if (((rand1-prob0) < prob1)&&(Ep>Eth_Star)) 
  {
    _NormalOrStar = true;
    return true;
  }
  else 
    return false;   
}

double SimLiT::CalculateDeltaECm()
{
  double Ecm = _ReactionEnergy*(Mcm/Mp);
  double Qvalue = 1644.2396; //keV
  double Qvalue_Star = 2075.0; //keV
  
  if(_NormalOrStar)
    return Ecm-Qvalue_Star;
  else
    return Ecm-Qvalue;
}

void SimLiT::GenerateNeutronVelocity(double theta)
{
  double phi; // radians
  // phi = 2*pi*rand()/(RAND_MAX*1.0);  // phi distribute uniformly 
  phi = 2 * pi * G4UniformRand();  // phi distribute uniformly
  double absVcm;   // abs velocity in CM 
  double EnCM = _deltaECm*MBe/(Mn+MBe);           // The energy that goes to the neutron in CM frame
  absVcm =  sqrt(2*EnCM/Mn);
  _Vcm[0] = absVcm*sin(theta)*cos(phi);
  _Vcm[1] = absVcm*sin(theta)*sin(phi);
  _Vcm[2] = absVcm*cos(theta);
}

void SimLiT::DetermineReactionParameters(double Ep)
//TODO: Here is the main problem
{
  //DEBUG:
  debug_Ncalls++;

  double deltaE,N,Theta;

  deltaE = getDeltaE(Ep);
    
  double E = 0;
  double rand1,rand2,y;
  
  while (E==0)
  {
    debug_Ntries++;

    if(_NormalOrStar)
    {
      // Excited state reaction
      // rand1 = rand()/(RAND_MAX*1.0);
      rand1 = G4UniformRand();
      if ((Ep-deltaE)>Eth_Star)
	      E = Ep - deltaE*(1-rand1); 
      else
	      E = Eth_Star + (Ep-Eth_Star)*rand1;

      N = 7*GetDistribution(Ep,pi,2)*2*pi;
      // rand2 = rand()/(RAND_MAX*1.0);
      rand2 = G4UniformRand();
      Theta = pi*rand2;
      // y = N*rand()/(RAND_MAX*1.0);
      y = N * G4UniformRand();
      if (y>(2*pi*sin(Theta)*GetDistribution(E,Theta,2)))
        E = 0;

      //DEBUG:
      if (N<2*pi*sin(Theta)*GetDistribution(E,Theta,2))
      {
        debug_Noverflow++;
        //printf("err: overflow!\n");
      }
    }
    else
    {
      // Ground state reaction
      // rand1 = rand()/(RAND_MAX*1.0);
      rand1 = G4UniformRand();
      if ((Ep-deltaE)>Eth)
	      E = Ep - deltaE*(1-rand1); 
      else
	      E = Eth + (Ep-Eth)*rand1;

      N = 7*GetDistribution(Ep,pi,1)*2*pi;
      // rand2 = rand()/(RAND_MAX*1.0);
      rand2 = G4UniformRand();
      Theta = pi*rand2;
      // y = N*rand()/(RAND_MAX*1.0);
      y = N * G4UniformRand();
      if (y>(2*pi*sin(Theta)*GetDistribution(E,Theta,1)))
        E = 0;

      if (N<2*pi*sin(Theta)*GetDistribution(E,Theta,1))
      {
        debug_Noverflow++;
        //printf("err: overflow!\n");
      }
    }    
  }  

  _ReactionEnergy = E;
  _deltaECm = CalculateDeltaECm();
  GenerateNeutronVelocity(Theta);  
}

void SimLiT::TransformCMToLab()
{
  double VcmInlab;  //The velocity of the CM frame in the lab frame
  VcmInlab = (Mcm/MLi)*sqrt(2*_ReactionEnergy/Mp);
  _Vlab[0] = _Vcm[0];
  _Vlab[1] = _Vcm[1];
  _Vlab[2] = _Vcm[2] + VcmInlab;
}

void SimLiT::ClaculateNeutronEnergy()
{
  _neutronEnergy = 0.5*Mn*(pow(_Vlab[0],2)+pow(_Vlab[1],2)+pow(_Vlab[2],2));
}

void SimLiT::CalculateNeutronTheta()
{
 double absV;  //abs velocity
 absV = sqrt(2*_neutronEnergy/Mn);
 _neutronTheta = acos(_Vlab[2]/absV);
}

// double SimLiT::PrintCDFvectors()
// {
//   int i;
//   for (i=0;i<RESOLUTION*8;i++)
//     printf("%d %.0f %.0f %e %e %e %e %e %e\n",i,_BeamEnergy-5*_BeamSigma+(double)i*_BeamSigma/RESOLUTION,getDeltaE(_BeamEnergy-5*_BeamSigma+(double)i*_BeamSigma/RESOLUTION),_CDF_vectors[0][i][0],_CDF_vectors[1][i][0],_CDF_vectors[0][i][1],_CDF_vectors[1][i][1],_CDF_vectors[0][i][2],_CDF_vectors[1][i][2]);
// }

double SimLiT::PrintCDFvectors()
{
  int i;
  for (i = 0; i < RESOLUTION*8; i++) {
    G4cout << i << " " 
           << _BeamEnergy - 5*_BeamSigma + (double)i * _BeamSigma/RESOLUTION << " "
           << getDeltaE(_BeamEnergy - 5*_BeamSigma + (double)i * _BeamSigma/RESOLUTION) << " "
           << _CDF_vectors[0][i][0] << " " << _CDF_vectors[1][i][0] << " "
           << _CDF_vectors[0][i][1] << " " << _CDF_vectors[1][i][1] << " "
           << _CDF_vectors[0][i][2] << " " << _CDF_vectors[1][i][2] << G4endl;
  }
  return 0;
}

double SimLiT::getStoppingPower(double Ep)
{
  return StoppingPower[_composition][0] +
         StoppingPower[_composition][1]*Ep +
         StoppingPower[_composition][2]*Ep*Ep +
         StoppingPower[_composition][3]*Ep*Ep*Ep;         
}

double SimLiT::calculateDeltaE(double Ep)
{
  double z = GetTargetThickness()/um;
  double energy = Ep;
  double eps = 1e-3;
  for (double x=0;x<z;x+=eps)
  {
    energy = energy - getStoppingPower(energy)*eps;
    if (energy<Eth)
      break;
  }     
  return Ep-energy;     
}

void SimLiT::ConstructDeltaEVector()
{
  int i;
  double E_eps = _BeamSigma/(RESOLUTION*1.0); // keV  
  for (double energy=Eth;energy<(_BeamEnergy+3*_BeamSigma+0.1);energy=energy+E_eps)
  {
    i = (int) floor((energy-(_BeamEnergy-5*_BeamSigma))/E_eps);
      if ((i>=0)&&(i<=(8*RESOLUTION)))
        deltaEVector[i] = calculateDeltaE(energy);
  }
}

double SimLiT::getDeltaE(double Ep)
{
  //return calculateDeltaE(Ep);
  
  int e;
  double step;
  step = _BeamSigma/(RESOLUTION*1.0);
  e = (int) floor((Ep-(_BeamEnergy-5*_BeamSigma))/step);
  if (e<0) e = 0;
  else if (e>(8*RESOLUTION)) e = (8*RESOLUTION);

  return deltaEVector[e];
  
}

double SimLiT::getDeltaEReversed(double Ep, double eps)
{
  double e0 = Ep;
  double e1 = 4100;
  double e2 = (e0+e1)/2.0;
  double temp;
  int i = 0;
  while (fabs(e2-getDeltaE(e2)-Ep)>eps)
  {
    if (e2-getDeltaE(e2)-Ep>0)
    {
      temp = e2;
      e2 = (e0+e2)/2.0;
      e1 = temp;
      if (fabs(e2-e1)<eps)
        break;
    }
    else if (e2-getDeltaE(e2)-Ep<0)
    {
      temp = e2;
      e2 = (e1+e2)/2.0;
      e0 = temp;
      if (fabs(e2-e0)<eps)
        break;
    }
    else  
      break;
  }

  return getDeltaE(e2);
}
