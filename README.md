# University of Kentucky Accelerator Lab (UKAL) Quasi-Stellar Neutron Simulations
SimLiT and Geant4 simulations for stellar neutron generation and detection. Simulated quasi-stellar neutrons are generated via the SimLiT code developed by M. Friedman, et. al. (see their paper [here](https://doi.org/10.1016/j.nima.2012.09.027)). Resultant event-by-event output files are used as the neutron source for Geant4 simulations to track neutron energy and angular distributions as they pass through materials present in the physical design of the neutron detection housing.


## Cloning the Repository
Windows Users:
1. Download the zip file (under the green Code dropdown at the top).
2. Extract the repository to your desired location.

Linux Users:
1. Install Git using `sudo apt install git`.
2. Within your desired install directory, run `git clone https://github.com/sslerose/UKAL_Neutron_Simulation.git`.


## Installing Geant4
Windows Users:

Linux Users:

Installation macros are included in the Geant4 folder to perform most of the setup. Follow these steps (adapted from Physics Matters [YouTube tutorial](https://youtu.be/4DTumUo3IKw?si=EoMsXBIljGOl0YsK)):
1. Open a terminal and install relevent dependencies:
  
  ```bash
  sudo apt install wget cmake cmake-curses-gui g++ binutils libx11-dev libxpm-dev libxft-dev libxext-dev libglew-dev \
  libjpeg-dev libpng-dev libtiff-dev libgif-dev libxml2-dev libssl-dev libfftw3-dev libqt5core5a libxmu-dev
  ```
2. In the home user directory (~), create the file structure for the installation:

	```bash
	mkdir Software && cd Software && mkdir Geant4 && cd Geant4
	```
3. Clone the Geant4 installer:
    1. Go to the [Geant4 webpage](https://geant4.web.cern.ch/) and click Download at the top.
    2. Right-click "Download tar.gz" under "Source code" and copy the link.
    3. In your terminal, type `wget` followed by a space, then paste the download link and execute. It will look like the example below. (If you do not have `wget`, install it with `sudo apt install wget`).
       
	   ```bash
	   wget https://gitlab.cern.ch/geant4/geant4/-/archive/v11.3.2/geant4-v11.3.2.tar.gz
	   ```
4. Extract the installation:
   
	```bash
	tar xvfv geant4-v11.3.2.tar.gz
	```
5. Make a build directory:
   
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
 8. Make an alias to the Geant4 shell script:
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
    3. Before building or running any Geant4 program, you must call the alias to source the shell script:
       
       ```bash
       geant4make
       ```
