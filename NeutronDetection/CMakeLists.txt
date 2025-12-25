#----------------------------------------------------------------------------
# Setup the project

cmake_minimum_required(VERSION 3.16...4.1)
project(NeutronDetection)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default

option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# Setup Geant4 include directories, compile definitions, and ROOT functions

include(${Geant4_USE_FILE})
#include(${ROOT_USE_FILE})

#----------------------------------------------------------------------------
# Locate sources and headers for this project

include_directories(${PROJECT_SOURCE_DIR}/include 
                    ${Geant4_INCLUDE_DIR})
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(NeutronDetection NeutronDetection.cc ${sources} ${headers})
target_link_libraries(NeutronDetection ${ROOT_LIBRARIES} ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy all scripts to the build directory, i.e. the directory in which we
# build B1. This is so that we can run the executable directly because it
# relies on these scripts being in the current working directory.
#
set(NeutronDetection_SCRIPTS
  init_vis.mac
  vis.mac
  run_test.mac
  )

foreach(_script ${NeutronDetection_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()
