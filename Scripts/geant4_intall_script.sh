# Escape from shell on command fail
set -e


# Install dependencies
echo "Installing dependencies..."
sudo apt install -y wget cmake cmake-curses-gui g++ binutils libx11-dev \
libxpm-dev libxft-dev libxext-dev libglew-dev libjpeg-dev libpng-dev \
libtiff-dev libgif-dev libxml2-dev libssl-dev libfftw3-dev libqt5core5a \
libxmu-dev libxerces-c-dev


# Create file structure
if [ -d ~/Software/GEANT4 ]; then
	echo "~/Software/GEANT4 exists, entering directory..."
	cd ~/Software/GEANT4
elif [ -d ~/Software ]; then
	echo "~/Software exists, entering directory and creating /GEANT4..."
	cd ~/Software && mkdir GEANT4 && cd GEANT4
else
	echo "Creating directory ~/Software/GEANT4..."
	cd ~ && mkdir Software && cd Software && mkdir GEANT4 && cd GEANT4
fi


# Get and extract Geant4 installer
read -p "Enter your Geant4 version (eg., 11.3.2): " $VERSION
echo "Getting Geant4 installer..."
wget https://gitlab.cern.ch/geant4/geant4/-/archive/v$VERSION/geant4-v$VERSION.tar.gz

echo "Extracting installer..."
tar xzfv geant4-v$VERSION.tar.gz


# Make build directory
echo "Making build directory..."
mkdir geant4-v$VERSION-build && cd geant4-v$VERSION-build


# Configure and install
echo "Configuring install..."
cmake -DCMAKE_INSTALL_PREFIX=/home/Software/GEANT4/geant4-v$VERSION-install -DGEANT4_BUILD_MULTITHREADED=ON -DGEANT4_INSTALL_DATA=ON -DGEANT4_USE_OPENGL_X11=ON -DGEANT4_USE_QT=ON -DGEANT4_USE_SYSTEM_EXPAT=ON ../geant4-v$VERSION

echo "Installing..."
make -j && make install
