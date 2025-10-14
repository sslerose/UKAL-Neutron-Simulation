# University of Kentucky Accelerator Lab (UKAL) Quasi-Stellar Neutron Simulations

SimLiT and Geant4 simulations for stellar neutron generation and detection. Simulated quasi-stellar neutrons are generated via the SimLiT code developed by M. Friedman, et. al. (see their paper [here](https://doi.org/10.1016/j.nima.2012.09.027)). Resultant event-by-event output files are used as the neutron source for Geant4 simulations to track neutron energy and angular distributions as they pass through materials present in the physical design of the neutron detection housing.


## Cloning the Repository

**Windows Users:**
1. Download the zip file (under the green Code dropdown at the top).
2. Extract the repository to your desired location.

**Linux Users:**
1. Install Git using `sudo apt install git`.
2. Within your desired install directory, run `git clone https://github.com/sslerose/UKAL_Neutron_Simulation.git`.


## Installing Geant4

**Windows Users:**

**Linux Users:**  
A shell file for easy installation is included in the [Scripts](Scripts) directory. You can source (i.e., execute) the script as
```bash
cd /path/to/UKAL-Neutron-Simulation
source geant4_install_script.sh
```
where `/path/to` is the directory you cloned this repository to. It will prompt you for the current install version of Geant4, so you should have this handy. Find it by clicking "Download" at the top of the [Geant4 webpage](https://geant4.web.cern.ch/) to open the download page (it will be a number like 11.3.2).

If you would prefer to install manually, follow these steps (adapted from Physics Matters [YouTube tutorial](https://youtu.be/4DTumUo3IKw?si=EoMsXBIljGOl0YsK)):
1. Open a terminal and install relevent dependencies:  
    ```bash
    sudo apt install wget cmake cmake-curses-gui g++ binutils libx11-dev libxpm-dev libxft-dev libxext-dev libglew-dev libjpeg-dev  \
    libpng-dev libtiff-dev libgif-dev libxml2-dev libssl-dev libfftw3-dev libqt5core5a libxmu-dev libxerces-c-dev \
    qtbase5-dev qtbase5-dev-tools qtchooser qt5-qmake
    ```
2. In the home user directory (~), create the file structure for the installation:  
	```bash
	mkdir Software && cd Software && mkdir GEANT4 && cd GEANT4
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
5. Make the build directory:  
	```bash
	mkdir geant4-v11.3.2-build && cd geant4-v11.3.2-build
	```
6. Configure the build:  
	```bash
	ccmake ../geant4-v11.3.2
	```
    1. When the CMake GUI pops up, type <kbd>C</kbd> to Configure.
    2. Once finished, type <kbd>E</kbd> to enter the build selections.
    3. Change CMAKE_INSTALL_PREFIX to `/home/username/Software/GEANT4/geant4-v11.3.2-install` (remember to change the version number if yours is different).
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

## Sourcing the Geant4 Shell Script

**Linux Users:**  
Before you can build or run any Geant4 simulations, you must source the Geant4 libraries:

1. Open `.bashrc`:  
   ```bash
   sudo nano ~/.bashrc
   ```

2. At the end of the bash file, insert the alias and a header:  
   ```bash
   # Geant4 source alias
   alias geant4make="source /home/username/Software/Geant4/geant4-v11.3.2-install/share/Geant4/geant4make/geant4make.sh"
   ```
   Press <kbd>Ctrl</kbd> + <kbd>X</kbd> to exit the file, press <kbd>Y</kbd> to accept changes, and press <kbd>Enter</kbd> to write to the file.
   
   (Optional)
   If you want some feedback as to whether your file has been sourced properly, insert the following, more detailed function and alias:
   ```bash
    # <<< geant4 initialize >>>
	setup_geant4() {
    	# Check if GEANT4 is already set up
    	if [[ "$GEANT4_ACTIVE" == "true" ]]; then
        	echo "GEANT4 environment already active"
        	return 0
    	fi
    
    	# Source GEANT4 environment
    	source /home/sslerose/Software/GEANT4/geant4-v11.3.2-install/share/Geant4/geant4make/ge>
    
    	# Mark GEANT4 as active
    	export GEANT4_ACTIVE=true
    
    	echo "GEANT4 environment activated with system Qt (if available)"
    	echo "Final LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
	}
	alias geant4make="setup_geant4"
	# <<< geant4 initialize >>>
   ```
3. Before building or running any Geant4 program, call the alias:  
   ```bash
   geant4make
   ```
   It is *not* recommended that you make this alias activate on startup.

## Testing a Basic Example

*Ensure you have sourced the Geant4 libraries (i.e., called the alias* `geant4make`*) before performing this step.*
Although you should have received an error if any of the above failed, it is best to check that everything is working by building and running a basic example, B1.

**Linux Users:**  
I could offer a script for this section, but this particular set of actions must be repeated when building any project from source, so it is best to perform the steps manually as practice:
1. Navigate to the basic examples directory:  

   ```bash
   cd ~/Software/GEANT4/geant4-v11.3.2-install/share/Geant4/examples/basic/B1
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
<img width="700" alt="image" src="https://github.com/user-attachments/assets/2c877e07-20e3-436e-95f2-8c81cf44b3f8" />

(If you experience ghosting of the Geant4 window (i.e., the visualizer is transparent and duplicates when the window is moved), you will need to change your `.bashrc` file to force X11 display for Geant4. Return to **Sourcing the Geant4 Shell Script**, and in step (2), use the Optional sourcing method with
```bash
# <<< geant4 initialize >>>
setup_geant4() {
	# Check if GEANT4 is already set up
	if [[ "$GEANT4_ACTIVE" == "true" ]]; then
		echo "GEANT4 environment already active"
		return 0
	fi
	
    export QT_QPA_PLATFORM=xcb
    export GDK_BACKEND=x11\
    export WAYLAND_DISPLAY=""
	
	# Source GEANT4 environment
	source /home/sslerose/Software/GEANT4/geant4-v11.3.2-install/share/Geant4/geant4make/ge>
	
	# Mark GEANT4 as active
	export GEANT4_ACTIVE=true
	
	echo "GEANT4 environment activated with system Qt (if available)"
	echo "Final LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
}
alias geant4make="setup_geant4"
# <<< geant4 initialize >>>
```
To generalize the above to any project, just change step (1) to whichever directory the project is in, follow steps (2) -- (4) as normal, then launch using `./executable_name`, where the executable's name will be listed in the terminal after it has been created.
