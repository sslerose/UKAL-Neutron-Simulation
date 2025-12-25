# Set environment variables
export XDG_RUNTIME_DIR=/tmp/$UID
if [ ! -d /tmp/$UID ]; then
	mkdir -p $XDG_RUNTIME_DIR
fi
chmod 700 $XDG_RUNTIME_DIR

# Set default visualization driver and display variables
export G4VIS_DEFAULT_DRIVER=TSG_QT_ZB

export QT_X11_NO_MITSHM=1 && export LIBGL_ALWAYS_INDIRECT=1 && export LIBGL_DIR3_DISABLE=1

# Create singularity variables
export SING_RUN="singularity run --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf"

export BUILD_RUN="singularity run -B /tmp/.X11-unix:/tmp/.X11-unix --env DISPLAY=$DISPLAY,XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR,QT_X11_NO_MITSHM=$QT_X11_NO_MITSHM,LIBGL_ALWAYS_INDIRECT=$LIBGL_ALWAYS_INDIRECT,LIBGL_DIR3_DISABLE=$LIBGL_DIR3_DISABLE --app geant41132root6344 /share/singularity/images/ccs/conda/amd-conda26-rocky9.sinf"
