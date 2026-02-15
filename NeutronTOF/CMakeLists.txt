#------------------------------------------------------------------------#
# Setup the project
#------------------------------------------------------------------------#

cmake_minimum_required(VERSION 3.16...4.1)
project(NeutronTimeOfFlight)


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

add_executable(NeutronTimeOfFlight NeutronTimeOfFlight.cc ${sources} ${headers})
target_link_libraries(NeutronTimeOfFlight ${ROOT_LIBRARIES} ${Geant4_LIBRARIES})


#------------------------------------------------------------------------#
# Copy scripts to the build directory
#------------------------------------------------------------------------#

# Copy visualization and analysis scripts into build/
set(NeutronTimeOfFlight_VIS_ANALYSIS_SCRIPTS
  vis.mac
  vis_export.mac
  vis_preview.mac
  AnalyzeTOF.C
  AnalyzeEff.C
  )

foreach(_script ${NeutronTimeOfFlight_VIS_ANALYSIS_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
    )
endforeach()


# Copy test scripts from source root into build/Data_Test/
set(NeutronTimeOfFlight_TEST_SCRIPTS
  run_test.mac
  run_nTest.sh
  )

foreach(_script ${NeutronTimeOfFlight_TEST_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/Data_Test/${_script}
    COPYONLY
    )
endforeach()


# Copy efficiency scripts from source root into build/Data_Efficiency/
set(NeutronTimeOfFlight_EFFICIENCY_SCRIPTS
  run_efficiency.mac
  efficiency.mac
  run_dEff.sh
  )

foreach(_script ${NeutronTimeOfFlight_EFFICIENCY_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/Data_Efficiency/${_script}
    COPYONLY
    )
endforeach()


# Copy TOF scripts from source root into build/Data_TOF/
set(NeutronTimeOfFlight_TOF_SCRIPTS
  run_angles.mac
  angles.mac
  run_nTOF.sh
  )

foreach(_script ${NeutronTimeOfFlight_TOF_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/Data_TOF/${_script}
    COPYONLY
    )
endforeach()
