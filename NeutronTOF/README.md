# Neutron Time-of-Flight (TOF) Simulation


## Building the Project

### Locally

**Linux:**  
The steps to run this project are essentially identical to those for the basic example (as they are for all projects). Again, source the Geant4 libraries before starting (`geant4make`).

1. Create a directory for your projects:

   ```bash
   cd ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4/examples
   mkdir my_projects
   ```
2. Clone the repo to your projects directory:
   ```bash
   git clone https://github.com/sslerose/UKAL-Neutron-Simulation.git
   ```
3. Enter the UKAL-Neutron-Simulation folder and make a build directory:
   ```bash
   cd ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4/examples/my_projects/UKAL-Neutron-Simulation
   mkdir build && cd build
   ```
4. Create the Makefile:  
   ```bash
   cmake ..
   ```
5. Make the simulation executable:
   ```bash
   make
   ```


### On the MCC

1. Verify that steps (1) and (2) from [running the basic example](../README.md#building-and-running-a-basic-example-1) on the cluster have been performed.
2. Within the Geant4 projects directory, clone the repo:

    ```bash
    git clone https://github.com/sslerose/UKAL-Neutron-Simulation.git
    ```
2. Enter the repo directory:

	```bash
	cd UKAL-Neutron-Simulation
	```
3. Source the build script to create the Makefile and simulation executable:
	```bash
	. build_UKAL_ND.sh
	```


## Running the Project


### Commands

There are several commands to change the properties of the detector and particle generator.

**Detector:**  
`/neutronTOF/det/setDetectorDistance` : set the radial distance of the detector from the particle source.  
`/neutronTOF/det/setDetectorAngle` : set the angle of the detector about the y-axis.
`/neutronTOF/det/setSpanningStartAngle` : *(for cross-section visualization only)* set the initial spanning angle of the cylindrical detector assembly.  
`/neutronTOF/det/setSpanningStartAngle` : *(for cross-section visualization only)* set the final spanning angle of the cylindrical detector assembly.  
`/neutronTOF/det/setWorldMaterial` : set the world material of the simulation.  
`/neutronTOF/det/printParameters` : print the current detector parameters including materials and geometry.

**Particle Generator:**  
`/neutronTOF/gun/useSimLiT` : enable/disable SimLiT as the neutron source.  
`/neutronTOF/gun/beamEnergy` : set the proton beam energy for the SimLiT source.  
`/neutronTOF/gun/beamSigma` : set the proton beam energy spread for the SimLiT source.  
`/neutronTOF/gun/targetMaterial` : set the target lithium compound for the SimLiT source.  
`/neutronTOF/gun/targetThickness` : set the target thickness for the SimLiT source.  
`/neutronTOF/gun/energy` : set the neutron beam energy for the simple gun source.  
`/neutronTOF/gun/limitToDetector` : limit the generation of neutrons by SimLiT to a region around the detector (use only in a vacuum).  
`/neutronTOF/gun/printParameters` : print the current particle generator parameters.  
`/run/beamOn ###` : generate ### events, one neutron per event.


### Interactive Mode

From the build directory, launch the project locally using  
```bash
./NeutronTimeOfFlight
```
or from the MCC using
```bash
$BUILD_RUN ./NeutronTimeOfFlight
```
In either session, try some of the available commands either by typing them in the console or using the command dropdowns on the left-hand side. If using the dropdown, selecting a command will open a guidance box for its use and acceptable inputs.

When generating events in interactive mode, limit the number of events to only a few hundred, though 10 or 20 will do just as well to see the effect of commands. Neutron paths will be displayed in green, while interactions with the Li6 detector (collisions, fission, etc.) will show as blue dots. Any instance of a $^6\text{Li}(n,t)\alpha$ process will show as a red dot, though only about 5% of neutrons that enter the detector will produce a reaction.


### Batch Mode

For most cases, it is better to run the simulation in batch mode using a macro file.

**Locally:**  
To run batch mode locally, just add the desired macro file after the execution call:  
```bash
./NeutronTimeOfFlight your_macro.mac
```

**MCC:**  
Running batch mode on the MCC requires using Slurm (Simple Linux Utility for Resource Management) via a shell script. Any of the `run_nXXX.sh` scripts provides an outline for Slurm jobs with the following structure:  
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

singularity run --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf ../NeutronTimeOfFlight your_macro.mac
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

Note that the execution call `../NeutronTimeOfFlight` in the shell scripts have two leading periods, which directs the terminal to look in the parent directory for the executable. These batch scripts will write large data sets from the simulation and intended to be called within a folder that will hold the generated ROOT files. This keeps your different data sets organized within your build directory and removes potential analysis error (as discussed below). The macro file called by the shell script (and any dependent macro files) must be placed in the same folder as the shell script to execute properly.

As an example, the `run_efficiency.mac`, `efficiency.mac`, and `run_dEff.sh` files are copied into `build/Data_Efficiency` when the project is built. Running `run_dEff.sh` will use the macros from the same directory, but the executable from the parent `build/`.


## Test Run

Before doing more intensive simulations, we will want to make sure everything is working as expected by running the project using a test macro in batch mode either locally with `run_test.mac` or on the cluster with `run_nTest.sh` (see above for batch mode instructions). Both can be found in `build/Data_test`.

The macro generates 500,000 neutrons at 50 keV along the z-axis using a simple gun source towards the detector placed 50 cm away. Once complete, the simulation will output a ROOT file named `nTOF_Gun_50_0keV_0_0deg.root` with a TTree containing the recorded data which can be analyzed using ROOT. Open ROOT using
```bash
root -l
```
then load the analysis file and run its main analysis function:
```bash
.L AnalyzeTOF.C
analyzeTOF()
```
**NOTE:** On the MCC, you need to load ROOT before using it. To find the proper command, run
```bash
module spider root
```
It will be something like
```bash
module load ccs/conda/root/6.32.2
```
Then just follow the same steps from above.

The analysis program will produce several histograms and plots of the given ROOT files, all saved under default names as png files:

- `captures_histogram` : The distribution of neutron captures within the detector over detector angles.

- `efficiency_vs_angle` : The proportion of incident neutrons that are captured within the detector.

- `neutron_energy_distribution` : The distribution of generated neutron energy.

- `neutron_theta_distribution` : The distribution of generated neutron planar angle.

- `neutron_energy_vs_theta` : A density plot of a systematic sample of generated neutron energy vs planar angle.

- `tof_energy_per_angle` : The distribution of captured neutron energy as calculated by TOF, separated by detector angle.

- `tof_energy_spectrum` : The distribution of captured neutron energy as calculated by TOF.

- `tof_energy_vs_angle` : The mean TOF energy over detector angles.

The program will also create a text file of the results, providing numerical results to supplement the histograms and plots.

Most of the data from the test run is not insightful, but if the captures histogram and TOF energy spectrum are populated, then we know that everything is likely working correctly.


## Full TOF Simulation

*It is recommended that full simulation runs only be implemented on the MCC (assuming you have access) or some other high-performance cluster due to their computational intensity and large file outputs. As such, all implementation in this section is done with the MCC in mind.*

### Efficiency Profile

To create accurate TOF spectra, an efficiency profile of the implemented detector over the energy range of interest is required. From the `build/Data_Efficiency` directory, run the efficiency script with
```bash
sbatch run_dEff.sh
```
By default, this will generate 5 million mono-energetic neutrons at each energy from 1 to 150 keV in 1 keV increments. The detector is placed at 0 degrees in a vacuum to prevent scattering before reaching the detector. Once complete, open ROOT in the `build` directory with
```bash
root -l
```
and analyze the efficiency data:
```bash
.L AnalyzeEff.C
AnalyzeEff("Data_Efficiency")
```
The analysis program will produce a log-log plot of the measured detector efficiency over the generated neutron energies, a results text file that gives you information on the efficiency analysis, and a text file of a table of values that we will use when analyzing the TOF data.


### TOF Run

To run the TOF simulation, enter the `build/Data_TOF` directory and run the TOF script:
```bash
sbatch run_nTOF.sh
```
By default, this will generate 500 million neutrons at each detector angle from 0 to 70 degrees in 5 degree increments. Detector placement can be changed in the `run_angles.mac` macro file, and neutron count in the `angles.mac` macro file. If the number of neutrons generated is increased, also increase the time limit of the simulation (roughly 30 minutes for every 100 million neutrons generated).

**NOTE:** It is recommended to initiate separate runs from within a dedicated folder to keep generated ROOT files separated and organized. To do this yourself, copy the relevant script and its dependent macro files into your new folder and run the simulation from there. All ROOT files generated will remain in that folder.


### Analysis

With the TOF run complete, copy the `efficiency_table.txt` file from the efficiency data folder into the TOF data folder. Open ROOT in the build directory and then analyze the TOF data:
```bash
.L AnalyzeTOF.C
AnalyzeTOF("Data_TOF")
```

If you created a new folder for your data, change the file path in the function call. There are also a few optional variables for the analysis, such that you can specify individual angular spectra to plot, include diagnostic plots for your TOF data, or change the number of threads used during the analysis:
```bash
analyzeTOF("path/to/data/directory", targetAngles = {10.0, 30.0, 50.0, 60.0}, includeDiagnostic = true, nThreads = 4)
```

*If the target angles do not exist in your data, the plots for these angles will be skipped in the analysis.*

This analysis will produce the same plots and results text file as seen during the test run, but the statistics now have meaning. You will also see normalized double-differential spectra of the expected and TOF-calculated neutron energy distributions for those target angles specified, as well as the total integral spectrum across all simulated angles.