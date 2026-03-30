#------------------------------------------------------------------------#
# Setup the project
#------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.16...4.1)
project(Activation)


#------------------------------------------------------------------------#
# Find Geant4 package, activating all available UI and vis drivers by default
#------------------------------------------------------------------------#

option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#------------------------------------------------------------------------#
# Setup Geant4 include directories, compile definitions, and ROOT functions
#------------------------------------------------------------------------#

include(${Geant4_USE_FILE})


#------------------------------------------------------------------------#
# Locate sources and headers for this project
#------------------------------------------------------------------------#

include_directories(${PROJECT_SOURCE_DIR}/include 
                    ${Geant4_INCLUDE_DIR})
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)


#------------------------------------------------------------------------#
# Add the executable, and link it to the Geant4 libraries
#------------------------------------------------------------------------#

add_executable(Activation Activation.cc ${sources} ${headers})
target_link_libraries(Activation ${ROOT_LIBRARIES} ${Geant4_LIBRARIES})


#------------------------------------------------------------------------#
# Copy scripts to the build directory
#------------------------------------------------------------------------#

# Copy visualization and analysis scripts into build/
set(Activation_VIS_ANALYSIS_SCRIPTS
  vis.mac
  vis_export.mac
  vis_preview.mac
  )

foreach(_script ${Activation_VIS_ANALYSIS_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()


# Copy test scripts from source root into build/Data_Test/
set(Activation_TEST_SCRIPTS
  )

foreach(_script ${Activation_TEST_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/Data_Test/${_script}
    COPYONLY
    )
endforeach()
