# Gamma Spectroscopy Simulation

The gamma spectroscopy simulation, GammaSpec, tracks radioisotope decays of generated particles and the subsequent interaction of gamma particles within two realistically-modeled high purity germanium (HPGe) detectors. An analysis file is provided to facilitate generation of pulse-height spectra (PHS) from the interaction data and perform subsequent photopeak analysis.


## Building the Project

### Locally

**Linux:**
The steps to run this project are essentially identical to those for the basic example (as they are for all projects). Again, source the Geant4 libraries before starting (`geant4make`).

1. Source the Geant4 libraries:

   ```bash
   geant4make
   ```
2. Enter the `UKAL-Neutron-Simulation/GammaSpec` folder and make a build directory:
   ```bash
   cd /path/to/my_projects/UKAL-Neutron-Simulation/GammaSpec
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

1. Enter the repo:

	```bash
	cd /scratch/user123/Geant4/UKAL-Neutron-Simulation
	```
2. Source the build script to create the Makefile and simulation executable:
	```bash
	. build_UKAL_GammaSpec.sh
	```


## Running the Project


### Commands

**General:**  
`/gammaSpec/setWorldMaterial` : set the world material.  
`/gammaSpec/printParameters` : print the current world, absorber, and detector parameters.

**Detector:**  
`/gammaSpec/det/setDetectorDistance` : set the radial distance of the detector from the origin.  
`/gammaSpec/det/setDetectorAngle` : set the angle of the detector about the y-axis.  
`/gammaSpec/det/setSpanningStartAngle` : *(for cross-section visualization only)* set the initial spanning angle of the cylindrical detector assembly.  
`/gammaSpec/det/setSpanningStartAngle` : *(for cross-section visualization only)* set the final spanning angle of the cylindrical detector assembly.

**Absorber:**  
`/gammaSpec/abs/setAbsorberMaterial` : set the isotopic material of the absorber by specifying the atomic number, mass number, and density of the isotope.  
`/gammaSpec/abs/setAbsorberThickness` : set the thickness of the absorber (does not change gold foils).  
`/gammaSpec/abs/setAbsorberRadius` : set the radius of the absorber and gold foils.  
`/gammaSpec/abs/setAbsorberSpanningStartAngle` : set the initial spanning angle of the absorber and gold foils.  
`/gammaSpec/abs/setAbsorberSpanningEndAngle` : set the final spanning angle of the absorber and gold foils.


### Interactive Mode

From the build directory, launch the project locally using
```bash
./GammaSpec
```
or from the MCC using
```bash
$BUILD_RUN ./GammaSpec
```


### Batch Mode

For most cases, it is better to run the simulation in batch mode using a macro file.

**Locally:**  
Add the desired macro file after the execution call:
```bash
./GammaSpec your_macro.mac
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

singularity run --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf ../GammaSpec your_macro.mac
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

Note that the execution call `../GammaSpec` in the shell scripts have two leading periods, which directs the terminal to look in the parent directory for the executable. All data written by the simulation will be saved to the directory of script execution. It is best practice to keep the macro file called by the shell script (and any dependent macro files) in the same directory as the shell script for execution and organization purposes.


## Test Run, Decay Modes, and Analysis

Before making your own simulation macros, you should test that the simulation is working and understand the sampling methods of the simulated radioactive decays.

### Cobalt-60 Test Run

Either locally or via the MCC, run GammaSpec in batch mode with `Data_Test/run_test.mac`. The macro performs two separate runs, each generating 1,000,000 isotopes of Cobalt-60 at rest at the origin within the absorber volume. The first runs in analogue decay mode, saving data to the file `co60_an.root`, and the second in variance reduction decay mode, saving data to `co60_vr.root`.

### Analogue Decays

In analogue mode, Geant4 samples the times of radioactive decays using standard Monte Carlo modeling. Under this method, all sampled decay times are considered "real" and daughter products are given statistical weights of 1. Geant4 simulates entire decay chains (this can be limited using `/process/had/rdm/nucleusLimits`), so any gamma spectra collected with this method represent infinite (or very large in comparison to the radioisotope's half-life) total collection times, which is not necessarily realistic in practice.

### Variance Reduction (VR) Decays

*VR mode is only valid when the radioisotope is considered at rest relative to the geometry at the time of decay.*

In VR mode, Geant4 biases radioactive decays to occur only within certain time windows. Here, the statistical weight of daughter products are reduced based on the probability of its parent decay occuring at the sampled time as determined by the decay rate of its parent. With this in mind, the user must supply a decay bias (`measures.data`) profile, and optionally a source time profile.

The decay bias profile defines the time windows in which radioactive decays are forced to occur within. This time window should mimic the total collection time of your gamma spectroscopy measurement.

The source time profile describes the time history of the source during the simulation (i.e., when primaries are generated relative to $t=0$) and is convolved with the decay rate. For GammaSpec, the radioisotope population has already been determined and is present at $t=0$, so the source time profile may be omitted.

### Analysis

*The analysis functions detailed below will not work if multiple windows are defined in the decay bias profile. This capability may be added in the future.*

With either ROOT data file from the test runs, we can calculate a pulse-height spectrum (PHS) from energy deposits in the HPGe detector and a gamma energy spectrum for those gammas that leave the absorber. We can also perform photopeak analysis on our PHS over energy ranges of interest.

From the `Data_Test` directory, open ROOT with
```bash
root -l
```
then load the analysis file and run the print functions command to see the available functions, their purpose, and their arguments:
```bash
.L AnalyzePHS.C
printFuncs()
```
The test runs simulate a single radioisotope, so the PHS and gamma spectrum of the analogue run, for example, are calculated by
```bash
analyzePHS("co60_an.root", 1.0, 2000, 2.0, true)
```
where the collection window, number of bins, and max energy arguments have been chosen for convenience. As an output, the function creates PNGs of the PHS and gamma spectrum with accompanying CSV files for bin-center and counts data, CSV files of a sample of the emitted particle and decay product populations, and a ROOT file of the resulting PHS (via the `true` parameter).

With the ROOT file of the PHS, you may also view the graph interactively by
```bash
viewPHS("co60_an_PHS_1.00us.root")
```
By clicking and dragging along energy axis, select a region around the first obvious peak near 1.2 MeV, and define an energy region that just captures the peak, say 1.168 - 1.178 MeV. Do the same for the second peak near 1.3 MeV, and define a region like 1.328 - 1.337 MeV. We can then perform photopeak analysis on both peaks simultaneously as
```bash
analyzePhoto("co60_an_PHS_1.00us.root", {{1.168, 1.178}, {1.328, 1.337}})
```

If we perform the same analysis on the VR data, the resulting spectra will depend on the time window defined in `measures.data`. With a half-life of approximately 5.25 years, a (reasonable) VR measurement of Cobalt-60 will generally require a bias window on the order of several hours. The PHS will look very similar, as there is no implementation of background noise.


## Multi-Isotope Simulation

*It is recommended that full simulation runs only be implemented on the MCC (assuming you have access) or some other high-performance cluster due to their computational intensity and large file outputs. As such, all implementation in this section is done with the MCC in mind.*

Multi-isotope simulations can be performed using a single macro. Each isotope is assigned to a separate run and its data is written to a separate ROOT file, and the collection of ROOT files generated can be analyzed to create a combined PHS.


### Creating a Run Info File

To have good statistics of a multi-isotope run, we want to generate a large population of each isotope. However, the individual PHS of each isotope needs to be scaled proportional to its true initial population before being added to the combined PHS. This PHS scaling is facilitied by a `runs.info` file (the name can be changed).

The info file is built by recording the true initial population and the initial population simulated in GammaSpec for each isotope. By true population, we mean the surviving population determined from a NeutronActivation simulation. Specifically, the surviving *radioactive* species. As an example, a 1-billion-particle run of NeutronActivation with an Iron-60 absorber, a beam time of 18 minutes, and a cooling time of 6 minutes, may yield populations

- Au-198: 259,427
- Fe-61: 871
- Mn-56: 177
- Mo-101: 125

We can then construct a `runs.info` file with the first column as the species, the second as the true initial population, and the third as the simulated population for GammaSpec:
```bash
au198 259427 1000000
fe61 871 1000000
mn56 177 1000000
mo101 125 1000000
```


### Running the Simulation

Using `Data_Test/run_test.mac` as an outline, a multi-isotope macro can be constructed as
```bash
# Set ion (Gold-198)
/gun/particle ion
/gun/ion 79 198

/analysis/setFileName au198

# Run events
/run/beamOn 1000000

# Set ion (Iron-61)
/gun/particle ion
/gun/ion 26 61

/analysis/setFileName fe61

# Run events
/run/beamOn 1000000

...
```
Each ion is defined by its atomic number and mass number, and the saved ROOT file is named identically to the `runs.info` population names.

**NOTE 1:** It is recommended to initiate independent runs from within a dedicated folder to keep generated ROOT files separated and organized. To do this yourself, copy the relevant script and its dependent macro files into your new folder and run the simulation from there. All ROOT files generated will remain in that folder.

**NOTE 2:** If you changed the geometry or material properties of the absorber in your preceeding NeutronActivation simulation, make the same changes in your GammaSpec macro using the available commands.


### Analysis

Any individual ROOT file can be analyzed in the same manner as the test run, or the set of ROOT files can be analyzed as a whole by
```bash
analyzePHSTotal("runs.info", "/path/to/rootfiles", "outStem", window_us, nBins, eMax, toPlot, saveRoot)
```
with all but the info file path being optional arguments:
- rootDir   - directory containing the .root files (default = ".")
- outStem   - output filename stem for the combined PHS (default = "Total")
- window_us - charge-collection window length in microseconds (default = 1.0)
- nBins     - number of histogram bins (default = 3000)
- eMax      - histogram upper edge in MeV (default = 3.0)
- toPlot    - also save the per-run PHS and gamma plots/CSVs (default = false)
- saveRoot  - also save the combined PHS as its own .root file (default = false)

Photopeak analysis can be further conducted on the combined PHS if one saves the resulting ROOT file (`saveRoot=true`).