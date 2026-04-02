# Establish cluster variables
. mcc_variables.sh

# Escape from shell on command fail
set -e

# Build directory
if [ -d NeutronActivation/build ]; then
	echo "build directory exists, entering.."
	cd NeutronActivation/build
else
	echo "Creating and entering build directory..."
	cd NeutronActivation && mkdir build && cd build
fi

# Prepare build files
echo "Running cmake..."
$SING_RUN cmake .. -DGeant4_DIR=$CONDA_PREFIX/lib/Geant4-11.3.2/cmake

# Make executable
echo "Running make..."
$SING_RUN make
