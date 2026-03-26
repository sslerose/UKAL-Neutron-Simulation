# Escape from shell on command fail
set -e


# Install dependencies
echo "Installing dependencies..."
sudo apt install -y wget cmake cmake-curses-gui g++ binutils libx11-dev \
libxpm-dev libxft-dev libxext-dev libglew-dev libjpeg-dev libpng-dev \
libtiff-dev libgif-dev libxml2-dev libssl-dev libfftw3-dev libxmu-dev \
libxerces-c-dev libgl1-mesa-dev mesa-utils libxkbcommon-dev \
qt6-base-dev qt6-base-private-dev qtchooser


# Create file structure
if [ -d ~/Software/Geant4 ]; then
	echo "~/Software/Geant4 exists, entering directory..."
	cd ~/Software/Geant4
elif [ -d ~/Software ]; then
	echo "~/Software exists, entering directory and creating /Geant4..."
	cd ~/Software && mkdir Geant4 && cd Geant4
else
	echo "Creating directory ~/Software/Geant4..."
	cd ~ && mkdir Software && cd Software && mkdir Geant4 && cd Geant4
fi


# Get and extract Geant4 installer
echo "Getting Geant4 installer..."
VERSION=11.4.1
wget https://gitlab.cern.ch/geant4/geant4/-/archive/v$VERSION/geant4-v$VERSION.tar.gz

echo "Extracting installer..."
tar xzfv geant4-v$VERSION.tar.gz


# Make build directory
echo "Making build directory..."
mkdir geant4-v$VERSION-build && cd geant4-v$VERSION-build


# Configure and install
echo "Configuring install..."
cmake -DCMAKE_INSTALL_PREFIX=/home/$USER/Software/Geant4/geant4-v$VERSION-install -DGEANT4_BUILD_MULTITHREADED=ON -DGEANT4_INSTALL_DATA=ON -DGEANT4_USE_OPENGL_X11=ON -DGEANT4_USE_QT=ON -DGEANT4_USE_SYSTEM_EXPAT=ON ../geant4-v$VERSION

echo "Installing..."
make -j && make install
