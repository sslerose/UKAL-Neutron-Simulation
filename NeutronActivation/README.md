# Neutron Activation Simulation

The quasi-stellar neutron activation simulation, *NeutronActivation*, generates and tracks neutrons into isotopic samples, measuring the population of radioisotopes over time as the sample is bombarded. Simulated neutrons are Maxwellian-distributed and defined using SimLiT, a Monte Carlo neutron generation simulation developed by M. Friedman, et. al. (see [here](https://doi.org/10.1016/j.nima.2012.09.027)). Using the $^7\text{Li}(p,n)^7\text{Be}$ reaction just above threshold ($E_p\approx1.9\,\text{MeV}$), the neutron distribution is similar to that of a thermal spectrum with $kT\sim25\,\text{keV}$. A lithium-6 glass detector is defined for purposes of neutron flux measurement (WIP). An analysis file is provided to facilitate determination of post-activation radioisotope populations.


## Table of Contents
- [Building the Project](#building-the-project)
  - [Locally](#locally)
  - [MCC](#on-the-mcc)
- [Running the Project](#running-the-project)
  - [Interactive Mode](#interactive-mode)
  - [Batch Mode](#batch-mode)
  - [Available Commands](#available-commands)
- [Iron-60 Test Run](#iron-60-test-run)
- [Analysis](#analysis)

## Building the Project

### Locally

**Linux:**
The steps to run this project are essentially identical to those for the basic example (as they are for all projects). Again, source the Geant4 libraries before starting (`geant4make`).

1. Source the Geant4 libraries:

   ```bash
   geant4make
   ```
2. Enter the `UKAL-Neutron-Simulation/NeutronActivation` folder and make a build directory:
   ```bash
   cd /path/to/my_projects/UKAL-Neutron-Simulation/NeutronActivation
   mkdir build && cd build
   ```
3. Create the Makefile:
   ```bash
   cmake ..
   ```
4. Make the simulation executable:
   ```bash
   make
   ```


### On the MCC

1. Enter the repo `Scripts` folder:

	```bash
	cd /scratch/user123/Geant4/UKAL-Neutron-Simulation/Scripts
	```
2. Source the build script to create the Makefile and simulation executable:
	```bash
	. build_UKAL_Activation.sh
	```


## Running the Project

### Interactive Mode

From the build directory, launch the project locally using
```bash
./NeutronActivation
```
or from the MCC using
```bash
$BUILD_RUN ./NeutronActivation
```


### Batch Mode

For most cases, it is better to run the simulation in batch mode using a macro file.

**Locally:**  
Add the desired macro file after the execution call:
```bash
./NeutronActivation your_macro.mac
```

**MCC:**  
Running batch mode on the MCC requires using Slurm (Simple Linux Utility for Resource Management) via a shell script. The `Data_Test/run_gSpec.sh` script provides an outline for Slurm jobs with the following structure:
```bash
#!/bin/bash
#SBATCH --time 00:30:00         # Time limit for the job (REQUIRED)
#SBATCH --job-name=JOB          # Job name
#SBATCH --ntasks=8              # Number of cores to allocate. Same as SBATCH -n
#SBATCH --partition=normal      # Partition/queue to run the job in. (REQUIRED)
#SBATCH -e slurm-%j.err         # Error file for this job.
#SBATCH -o slurm-%j.out         # Output file for this job.
#SBATCH -A NAME                 # Project allocation account name (REQUIRED)
#SBATCH --mail-type ALL         # Send email when job starts/ends
#SBATCH --mail-user EMAIL_ADDRESS  # Email address to send notifications to

singularity run --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf ../NeutronActivation your_macro.mac
```
Each of the hashed lines provide instructions to Slurm. There are four of primary importance that you will need to change:

- `--time` : The maximum time limit for the batch job. Longer time limits will be put lower in the Slurm queue, so be judicious.

- `--job-name` : The name of your job. This will show in email notifications.

- `-A NAME` : The project allocation name. Replace `NAME` with that which you used to start an interactive session via OOD (e.g., `coa_pi_uksr`).

- `--mail-user EMAIL_ADDRESS` : The email address to which Slurm notifications are sent. This is the recommended method for being notified of project start, completion, failure, or timeout.

Anything after the hashed lines will be executed as if it were in a terminal. Once a Slurm job ends, any output which would typically be shown in the terminal will be written into `slurm-#j.out`, and any errors with Slurm to `slurm-#j.err` , where `#` is the ID of the submitted job. To execute a job, call
```bash
sbatch your_slurm.sh
```

Note that the execution call `../NeutronActivation` in the shell scripts have two leading periods, which directs the terminal to look in the parent directory for the executable. All data written by the simulation will be saved to the directory of script execution. It is best practice to keep the macro file called by the shell script (and any dependent macro files) in the same directory as the shell script for execution and organization purposes.


### Available Commands

The following are commands available to use in an interactive session or as part of a macro. For information on parameters, open an interactive session and check command descriptions using the dropdowns in the left pane.

**General:**  
`/gammaSpec/setWorldMaterial` : set the world material.  
`/gammaSpec/printParameters` : print the current world, absorber, and detector parameters.

**Detector:**  
`/neutronAct/det/setDetectorDistance` : set the radial distance of the detector from the origin.  
`/neutronAct/det/setDetectorAngle` : set the angle of the detector about the y-axis.

**Absorber:**  
`/gammaSpec/abs/setAbsorberMaterial` : set the isotopic material of the absorber by specifying the atomic number, mass number, and density of the isotope.  
`/neutronAct/abs/setAbsorberDistance` : set the distance of the absorber from the origin along the $z$-axis.
`/gammaSpec/abs/setAbsorberThickness` : set the thickness of the absorber (does not change gold foils).  
`/gammaSpec/abs/setAbsorberRadius` : set the radius of the absorber and gold foils.  
`/gammaSpec/abs/setAbsorberSpanningStartAngle` : set the initial spanning angle of the absorber and gold foils.  
`/gammaSpec/abs/setAbsorberSpanningEndAngle` : set the final spanning angle of the absorber and gold foils.

**Particle Gun:**  
`/neutronAct/gun/useSimLiT` : enable or disable the use of SimLiT as the neutron source.  
`/neutronAct/gun/beamEnergy` : set proton beam energy for SimLiT source.  
`/neutronAct/gun/beamSigma` : set proton beam energy spread, 1-sigma.  
`/neutronAct/gun/targetMaterial` : set lithium target composition for SimLiT.  
`/neutronAct/gun/targetThickness` : set lithium target thickness for SimLiT.  
`/neutronAct/gun/energy` : set neutron energy for simple particle gun mode.  
`/neutronAct/gun/exposureTime` : set time exposure for event time randomization.  
`/neutronAct/gun/printParameters` : print current neutron source configuration.


## Iron-60 Test Run

Before making your own macros, you can test that the simulation is working using the example macro in `/Data_Test`. Either locally or via the MCC, run NeutronActivation in batch mode with `Data_Test/run_test.mac`. The macro defines an iron-60 absorber placed 5 mm from the neutron source (the origin) and generates 1 million neutrons using the default SimLiT configuration. The beam time of 900 seconds is chosen to bring the iron-61 ($T_{1/2}\approx6\,\text{min}$) population near saturation. The data is saved to the file `Fe60_Test.mac`.


## Analysis

With the ROOT file generated by a simulation run, a single function call will calculate the surviving radioisotope population after the defined beam time of the simulation and an optional cooling time.

From the directory containing your `.root` file, open ROOT with
```bash
root -l
```
then load the analysis file as
```bash
.L AnalyzeAct.C
```
and perform the population analysis:
```bash
analyzeAct("/path/to/data.root", beamTime, "timeScale", coolingTime, false)
```
The `beamTime` parameter should match that defined in your simulation macro, and the cooling time can be chosen to match your experimental design. The `"timeScale"` parameter defines the unit in which the beam and cooling times are defined. The Boolean flag at the end enables/disables generation of population histograms for which the surviving population after cooling is zero (most often these are excited states that decay promptly). Population curves for all species still present at the end of the cooling time are saved as images, with exact population values for all species being written to `surviving_populations.txt`.

To analyze the test data with a cooling time of 60 seconds, open ROOT in `/Data_Test` and call
```bash
analyzeAct("Fe60_Test.root", 900, "s", 60, false)
```
Note the shapes of the population curves for Au-198 and Fe-61, specifically.