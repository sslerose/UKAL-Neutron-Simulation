# University of Kentucky Accelerator Lab (UKAL) Quasi-Stellar Neutron Activation (Q-SNAc) Simulations

Geant4 particle simulations for quasi-stellar neutron generation, time-of-flight (TOF) analysis, and activation experiments. Simulated neutrons are generated via SimLiT, a Monte Carlo neutron generation simulation developed by M. Friedman, et. al. (see [here](https://doi.org/10.1016/j.nima.2012.09.027)). Resultant event-by-event output neutron data are used as the particle source for Geant4 simulations to track neutrons into a scintillation detection assembly for TOF analysis and activation in sample isotopes.


## Cloning the Repository

**Windows Users:**
1. Download the zip file (under the green Code dropdown at the top).
2. Extract the repository to your desired location.

**Linux:**
1. Install Git using `sudo apt install git`.
2. Within your desired install directory, run `git clone https://github.com/sslerose/UKAL-Neutron-Simulation.git`.


## Installing Geant4

**Windows Users:**

**Linux:**  
A shell file for easy installation is included in the [Scripts](Scripts) directory. You can source (i.e., execute) the script as
```bash
cd /path/to/UKAL-Neutron-Simulation
source geant4_install_script.sh
```
where `/path/to` is the directory you cloned this repository to. It will prompt you for the current install version of Geant4, so you should have this handy. Find it by clicking "Download" at the top of the [Geant4 webpage](https://geant4.web.cern.ch/) to open the download page (it will be a number like 11.3.2).

If you would prefer to install manually, follow these steps (adapted from Physics Matters [YouTube tutorial](https://youtu.be/4DTumUo3IKw?si=EoMsXBIljGOl0YsK)):
1. Open a terminal and install relevent dependencies:  
    ```bash
    sudo apt install wget cmake cmake-curses-gui g++ binutils libx11-dev libxpm-dev libxft-dev libxext-dev libglew-dev \
    libjpeg-dev libpng-dev libtiff-dev libgif-dev libxml2-dev libssl-dev libfftw3-dev libqt5core5a libxmu-de \
    libxerces-c-dev qtbase5-dev qtbase5-dev-tools qtchooser qt5-qmake
    ```
2. In the home user directory (~), create the file structure for the installation:  
	```bash
 	cd
	mkdir Software && cd Software && mkdir Geant4 && cd Geant4
	```
3. Get the Geant4 installer:
    1. Go to the [Geant4 webpage](https://geant4.web.cern.ch/) and click "Download" at the top.
    2. Right-click "Download tar.gz" under "Source code" and copy the link.
    3. In your terminal, type `wget` followed by a space, then paste the download link and execute. It will look like the example below. (If you do not have `wget`, install it with `sudo apt install wget`).
	
	   ```bash
	   wget https://gitlab.cern.ch/geant4/geant4/-/archive/v11.3.2/geant4-v11.3.2.tar.gz
	   ```
4. Extract the installation:

	```bash
	tar xzfv geant4-v11.3.2.tar.gz
	```
5. Make and enter the build directory:  
	```bash
	mkdir geant4-v11.3.2-build && cd geant4-v11.3.2-build
	```
6. Configure the build:  
	```bash
	ccmake ../geant4-v11.3.2
	```
    1. When the CMake GUI pops up, type <kbd>C</kbd> to Configure.
    2. Once finished, type <kbd>E</kbd> to enter the build selections.
    3. Change CMAKE_INSTALL_PREFIX to `/home/username/Software/Geant4/geant4-v11.3.2-install` (remember to change the version number if yours is different).
    4. Enable the following:
		  * BUILD_MULTITHREADED
        * INSTALL_DATA
        * USE_GDML
        * USE_OPENGL_X11
        * USE_QT
        * USE_SYSTEM_EXPAT
    5. Type <kbd>C</kbd> to Configure again. Once this is finished, type <kbd>G</kbd> to Generate. You will be returned to the main terminal once it is finished.
7. Build Geant4:
    1. From the build directory,  
	
       ```bash
       make -j#
       ```
       where the hash in `-j#` corresponds to the number of cores on your computer (run `lscpu | grep 'Core'` to check).
    2. Once the build is complete, run  
       ```bash
       make install
       ```


## Running Projects Locally

### Setup a Geant4 Environment Script

**Linux:**  
Before you can build or run any Geant4 simulations, you must source the Geant4 libraries:

1. Open `.bashrc`:  
   ```bash
   sudo nano ~/.bashrc
   ```

2. At the end of the bash file, insert the alias and a header:  
   ```bash
   # Geant4 source alias
   alias geant4make="source ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4/geant4make/geant4make.sh"
   ```
   Press <kbd>Ctrl</kbd> + <kbd>X</kbd> to exit the file, press <kbd>Y</kbd> to accept changes, and press <kbd>Enter</kbd> to write to the file.
   
   (Optional)
   If you want some feedback as to whether your file has been sourced properly, insert the following, more detailed function and alias:
   ```bash
    # <<< geant4 initialize >>>
	setup_geant4() {
    	# Check if Geant4 is already set up
    	if [[ "$GEANT4_ACTIVE" == "true" ]]; then
        	echo "Geant4 environment already active"
        	return 0
    	fi
    
    	# Source Geant4 environment
    	source ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4/geant4make/geant4make.sh
    
    	# Mark Geant4 as active
    	export GEANT4_ACTIVE=true
    
    	echo "Geant4 environment activated with system Qt (if available)"
    	echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
	}
	alias geant4make="setup_geant4"
	# <<< geant4 initialize >>>
   ```
3. Before building or running any Geant4 program, call the alias:  
   ```bash
   geant4make
   ```
   **NOTE:** It is *not* recommended that you make this alias activate on startup.


### Building and Running a Basic Example

Although you should have received an error if any of the above failed, it is best to check that everything is working by building and running a basic example, B1. Source the Geant4 libraries before starting (i.e., call `geant4make`).

**Linux:**  
I could offer a script for this section, but this particular set of actions must be repeated when building any project from source, so it is best to perform the steps manually as practice:
1. Navigate to the B1 project in the basic examples directory:  

   ```bash
   cd ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4/examples/basic/B1
   ```
2. Create a build directory:  
   ```bash
   mkdir build && cd build
   ```
3. Create the make file:  
   ```bash
   cmake ..
   ```
4. Make the simulation executable file:
   ```bash
   make
   ```
5. Launch the simulation:
   ```bash
   ./exampleB1
   ```

After a few moments, Geant4 will open in a new window. The largest section is the visualizer and is our main concern. You should see the following:  
<img width="600" alt="image" src="https://github.com/user-attachments/assets/2c877e07-20e3-436e-95f2-8c81cf44b3f8" />

If you experience ghosting of the Geant4 window (i.e., the visualizer is transparent and duplicates when the window is moved), you will need to change your `.bashrc` file to force X11 display for Geant4. Return to [Sourcing the Geant4 Shell Script](#sourcing-the-geant4-shell-script) and use the "Optional" sourcing method with
```bash
# <<< geant4 initialize >>>
setup_geant4() {
	# Check if Geant4 is already set up
	if [[ "$GEANT4_ACTIVE" == "true" ]]; then
		echo "Geant4 environment already active"
		return 0
	fi
	
    export QT_QPA_PLATFORM=xcb
    export GDK_BACKEND=x11\
    export WAYLAND_DISPLAY=""
	export G4VIS_DEFAULT_DRIVER=TSGQt
	
	# Source Geant4 environment
	source ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4/geant4make/geant4make.sh
	
	# Mark Geant4 as active
	export GEANT4_ACTIVE=true
	
	echo "Geant4 environment activated with system Qt (if available)"
	echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

	cd ~/Software/Geant4/geant4-v11.3.2-install/share/Geant4
}
alias geant4make="setup_geant4"
# <<< geant4 initialize >>>
```
where the four exports after the if statement have been added.

To generalize the above to any project, change step (1) to whichever directory the project is in, follow steps (2) -- (4) as normal, then launch using `./executable_name`, where the executable's name can be found in the project folder via a file explorer or calling `ls` in the terminal.


## Running Projects on the Morgan Compute Cluster (UKY Users Only)

The Morgan Compute Cluster (MCC) is a high-performance computational resource provided to certified users by the UKY Center for Computational Sciences. If you happen to be working under a PI with access to the MCC, you will be given access to the cluster for your projects.


### Connecting to the Cluster

The MCC provides remote desktop or batch-mode connections, each with their own benefits. (Note that the MCC is accessed by many users, and you should be mindful of your active working directory when managing projects.)

**(Recommended) Connecting via Remote Desktop**  
A remote desktop connection provides access to the cluster with visualization, allowing for start-to-finish project creation and testing just as you would on a personal machine. Access is provided via [Open OnDemand](https://mcc-ood.ccs.uky.edu/) (OOD) through your browser.
1. Log in to OOD using your LinkBlue credentials.
2. From the Interactive Apps menu at the top, select Morgan Compute Cluster (MCC).
3. Request a node:
	1. Account: `coa_pi_uksr`, where `pi` is the LinkBlue ID of your PI.
 	2. Hours: the number of hours you need continuous access.
	3. Cores: 4.
	4. Queue: Normal
4. Launch the node and wait for your session to start.

**Connecting via Secure Shell (SSH)**  
A simple SSH connection with X11 forwarding can be established via PuTTY or a terminal, but visualization is often quite slow. SSH connections are ideal for batch jobs, and X11 forwarding should only be used for quick visualization checks. If you are not connected to the UKY campus network (eduroam), you will need to connect to the campus VPN (see instructions [here](https://ukyrcd.atlassian.net/wiki/spaces/RCDDocs/pages/162103748/VPN+connection+to+UK+Campus+Resources)).

Using PuTTY:

Using terminal:
1. Connect via a simple ssh command:

	```bash
 	ssh -X linkblue@mcc.uky.edu
 	```
2. (WIP)


### Setup a Geant4 Environment Script

Geant4 is provided by a Singularity container with all libraries, datasets, visualization drivers, ROOT, and official examples. We will make a bash script to expedite this process.

If you're using the remote desktop, open a terminal in that instance. If you're accessing via SSH, you're ready to go.

1. Open `.bashrc` :

   ```bash
   nano ~/.bashrc
   ```
2. At the end of the bash file, create a setup function and alias:
   ```bash
   # <<< geant4 scripts >>>
   setup_geant4() {
      # Set environment variables
      export XDG_RUNTIME_DIR=/tmp/$UID
      if [ ! -d $XDG_RUNTIME_DIR ]; then
         mkdir -p $XDG_RUNTIME_DIR
      fi
      chmod 700 $XDG_RUNTIME_DIR

      # Set default visualization driver and display variables
      export G4VIS_DEFAULT_DRIVER=TSG_QT_ZB

      export QT_X11_NO_MITSHM=1 && export LIBGL_ALWAYS_INDIRECT=1 && export LIBGL_DIR3_DISABLE=1

      # Create singularity variables
      export SING_RUN="singularity run --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf"

      export BUILD_RUN="singularity run -B /tmp/.X11-unix:/tmp/.X11-unix --env DISPLAY=$DISPLAY,XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR,QT_X11_NO_MITSHM=$QT_X11_NO_MITSHM,LIBGL_ALWAYS_INDIRECT=$LIBGL_ALWAYS_INDIRECT,LIBGL_DIR3_DISABLE=$LIBGL_DIR3_DISABLE --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf"
   }
   alias geant4make="setup_geant4"
   # <<< geant4 scripts >>>
   ```
3. Source the bash file:
   ```bash
   source ~/.bashrc
   ```
   **NOTE:** Sourcing the `.bashrc` file and calling the alias `geant4make` must be done at the beginning of every new Geant4 session, just like when running projects locally.  
   **NOTE 2:** If `setup_geant4` already exists, change that command (and the corresponding alias) to `geant4_singularity` .


### Building and Running a Basic Example

Before importing any Neutron Simulation project, you should test that the basic B1 example works as expected. This will also give you a sanity check if future projects run into issues.

1. Enter your scratch directory:

   ```bash
   cd /scratch/user123
   ```
   where `user123` is your LinkBlue ID.  
   **NOTE:** Each user of the MCC has four possible directories to work in, each with different storage allocation and use cases. Familiarize yourself [here](https://ukyrcd.atlassian.net/wiki/spaces/RCDDocs/pages/162104005/File+System+Basics).
2. Make and enter a directory to house your Geant4 projects:	
	```bash
	mkdir Geant4 && cd Geant4
	```
3. Copy the B1 project folder and create a build folder:
	```bash
	$SING_RUN bash -c 'cp -r $CONDA_PREFIX/share/Geant4/examples/basic/B1 ./B1'
	cd B1
	mkdir build && cd build
	```
4. Make the example:
	```bash
	$SING_RUN cmake .. -DGeant4_DIR=$CONDA_PREFIX/lib/Geant4-11.3.2/cmake
	$SING_RUN make
	```
5. Run the example:
	```bash
	$BUILD_RUN ./exampleB1
	```
The same window seen when testing the basic example on your personal computer should pop up. If you experience a GLX (or other visualization) error, open the `vis.mac` file in the build folder (either using the file explorer in an interactive session or `nano` in the terminal) and verify `/vis/open` near the top of the file does *not* have a driver tag (like OGL or TSGQt), and retry step (6).
