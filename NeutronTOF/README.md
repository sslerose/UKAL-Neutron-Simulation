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
`/neutronTOF/det/printParameters` : print the current detector parameters including materials and geometry.

**Particle Generator:**  
`/neutronTOF/gun/useSimLiT` : enable/disable SimLiT as the neutron source.  
`/neutronTOF/gun/beamEnergy` : set the proton beam energy for the SimLiT source.  
`/neutronTOF/gun/beamSigma` : set the proton beam energy spread for the SimLiT source.  
`/neutronTOF/gun/targetMaterial` : set the target lithium compound for the SimLiT source.  
`/neutronTOF/gun/targetThickness` : set the target thickness for the SimLiT source.  
`/neutronTOF/gun/energy` : set the neutron beam energy for the simple gun source.  
`/neutronTOF/gun/printParameters` : print the current particle generator parameters.  
`/run/beamOn ###` : generate ### neutron events.


### Interactive Mode

From the build directory, launch the project locally using  
```bash
./NeutronTimeOfFlight
```
or from the MCC using
```bash
$BUILD_RUN ./NeutronTimeOfFlight
```
In either session, try some of the available commands either by typing them in console or using the command dropdowns on the left-hand side. If using the dropdown, selecting a command will open a guidance box for its use and acceptable inputs.

When generating events in interactive mode, limit the number of events to only a few hundred, though 10 or 20 will do just as well. Neutron paths will be displayed in green, while interactions with the Li6 detector (collisions, fission, etc.) will show as blue dots. Any instance of neutron capture will show as a red dot, but the likelihood of capture is so low (less than 1 in 10,000) that you will almost never see a capture within just a few hundred events.


### Batch Mode

For most cases, it is better to run the simulation in batch mode using a macro file.

**Locally:**  
To run batch mode locally, just add the desired macro file after the execution call:  
```bash
./NeutronTimeOfFlight your_macro.mac
```

**MCC:**  
Running batch mode on the MCC requires using Slurm (Simple Linux Utility for Resource Management) via a shell script. The `run_nTOF.sh` script provides an outline for Slurm jobs with the following structure:  
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

singularity run --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf ./NeutronTimeOfFlight your_macro.mac
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


## Output and Analysis

Within the build directory, there are three key macro files:

- `run_test.mac` : Generate 500,000 neutrons at 50 keV along the z-axis using a simple gun source towards the detector placed 50 cm away.

- `run_angles.mac` and `angles.mac` : Together, generate 1,000,000 neutrons each using SimLiT for detector configurations at a radial distance of 50 cm and angles about the y-axis between 0 and 90 degrees in 5 degree increments.

It is recommended to use the `run_angles.mac` and `angles.mac` set of macros only on the MCC (assuming you have access) or some other high-performance cluster due to their computational intensity and large file outputs.


### Test Run

Before doing more intensive simulations, run the project in batch mode with the `run_test.mac` macro either locally or on the cluster. Once complete, the simulation will output a ROOT file named `nTOF_Gun_50_0keV_0_0deg.root` with a TTree containing the recorded data which can be analyzed using ROOT. Open ROOT using
```bash
root -l
```
then load the analysis file and run its main analysis function:
```bash
.L AnalyzeData.C
analyzeData()
```
In the future, it will be useful to place the multiple ROOT files from a single macro (as will be the case for `run_angles.mac`) into their own folder. In such a case, add the file path to the function call:
```bash
analyzeData("path/to/data")
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

- `neutron_energy_vs_theta` : A scatterplot of a systematic sample of generated neutron energy vs planar angle.

- `tof_energy_spectrum` : The distribution of captured neutron energy as calculated by TOF.

- `tof_energy_per_angle` : The distribution of captured neutron energy as calculated by TOF, separated by detector angle.

- `tof_energy_vs_angle` : The mean TOF energy over detector angles.

The program will also create a txt file of the results, providing numerical results to supplement the histograms and plots.

Most of the data from the test run is not insightful, but the captures histogram and TOF energy spectrum, along with the corresponding results in the analysis text file, provide simulation results under optimal conditions (direct beam, moderate neutron energy with no variance).


### Full Run

Run the full TOF simulation using `run_angles.mac` . If using the cluster, change the macro file in `run_nTOF.sh` and increase the time limit (roughly 30 minutes for every 100 million neutrons generated).