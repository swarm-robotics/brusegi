################################################################################
# Configuration Options                                                        #
################################################################################
set(SILICON_WITH_ROBOT_RAB "NO" CACHE STRING "Enable robots to read/write over the RAB medium via sensors/actuators.")
set(SILICON_WITH_ROBOT_BATTERY "NO" CACHE STRING "Enable robots to use the battery.")
set(SILICON_WITH_ROBOT_LEDS "NO" CACHE STRING "Enable robots to use their LEDs.")
set(SILICON_WITH_ROBOT_CAMERA "YES" CACHE STRING "Enable robots to use their camera.")

define_property(CACHED_VARIABLE PROPERTY "SILICON_WITH_ROBOT_RAB"
  BRIEF_DOCS "Enable robots to use the RAB medium."
  FULL_DOCS "Default=NO.")
define_property(CACHED_VARIABLE PROPERTY "SILICON_WITH_ROBOT_BATTERY"
  BRIEF_DOCS "Enable robots to use the battery."
  FULL_DOCS "Default=YES.")
define_property(CACHED_VARIABLE PROPERTY "SILICON_WITH_ROBOT_LEDS"
  BRIEF_DOCS "Enable robots to use their LEDs."
  FULL_DOCS "Default=NO.")
define_property(CACHED_VARIABLE PROPERTY "SILICON_WITH_ROBOT_CAMERA"
  BRIEF_DOCS "Enable robots to use their camera."
  FULL_DOCS "Default=NO.")


# Needed by COSM for population dynamics and swarm iteration
if (NOT COSM_BUILD_FOR)
  set(COSM_BUILD_FOR "ARGOS_EEPUCK3D")
endif()

# Needed by COSM for population dynamics and swarm iteration
set(COSM_ARGOS_ROBOT_TYPE "e-puck")
set(COSM_ARGOS_ROBOT_NAME_PREFIX "ep")
set(COSM_ARGOS_CONTROLLER_XML_ID "epc")

################################################################################
# External Projects                                                            #
################################################################################
set(${target}_CHECK_LANGUAGE "CXX")


# Support libraries
add_subdirectory(ext/cosm)

if (${COSM_WITH_VIS})
  set(CMAKE_AUTOMOC ON)
  find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui)
  set(CMAKE_AUTOMOC OFF)
endif()

################################################################################
# Sources                                                                      #
################################################################################
if (NOT ${COSM_WITH_VIS})
    list(REMOVE_ITEM ${target}_ROOT_SRC)
endif()

################################################################################
# Libraries                                                                    #
################################################################################
# Define link libraries
set(${target}_LIBRARIES
  cosm
  ${cosm_LIBRARIES}
  rcppsw
  ${rcppsw_LIBRARIES}
  nlopt
  stdc++fs
  rt)

# Define link search dirs
set(${target}_LIBRARY_DIRS
  ${rcppsw_LIBRARY_DIRS}
  ${cosm_LIBRARY_DIRS})

if ("${COSM_BUILD_FOR}" MATCHES "ARGOS")
  set(argos3_LIBRARIES
  argos3core_simulator
  argos3plugin_simulator_entities
  argos3plugin_simulator_dynamics2d
  argos3plugin_simulator_qtopengl
  argos3plugin_simulator_media
  argos3plugin_simulator_genericrobot
  argos3plugin_simulator_epuck
  # argos3srocs_simulator_pipuck
  )

  set (${target}_LIBRARIES
    ${${target}_LIBRARIES}
    ${argos3_LIBRARIES}
    )

  set(${target}_LIBRARY_DIRS
    ${${target}_LIBRARY_DIRS}
    /usr/lib/argos3
    /usr/local/lib/argos3
    ${COSM_DEPS_PREFIX}/lib/argos3
    )
  if ("${COSM_BUILD_ENV}" MATCHES "MSI")
    # For nlopt
    set(${target}_LIBRARY_DIRS
      ${$target}_LIBRARY_DIRS}
      ${COSM_DEPS_PREFIX}/lib/argos3
      ${COSM_DEPS_PREFIX}/lib64)
  endif()
endif()

# Force failures at build time rather than runtime
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

link_directories(${${target}_LIBRARY_DIRS})
add_library(${target} SHARED ${${target}_ROOT_SRC})
add_dependencies(${target} rcppsw cosm)

################################################################################
# Includes                                                                     #
################################################################################
set(${target}_INCLUDE_DIRS
  "${${target}_INC_PATH}"
  ${rcppsw_INCLUDE_DIRS}
  ${cosm_INCLUDE_DIRS}
  /usr/include/eigen3)

set(${target}_SYS_INCLUDE_DIRS
  ${cosm_SYS_INCLUDE_DIRS}
  ${NLOPT_INCLUDE_DIRS})

target_include_directories(${target} PUBLIC ${${target}_INCLUDE_DIRS})

target_include_directories(${target} SYSTEM PUBLIC
  /usr/include/lua5.3 # Not needed for compiling, but for emacs rtags
  /usr/local/include
  ${nlopt_INCLUDE_DIRS}
  ${${target}_SYS_INCLUDE_DIRS}
  )

target_link_libraries(${target} ${${target}_LIBRARIES} cosm nlopt)

################################################################################
# Compile Options/Definitions                                                  #
################################################################################
if ("${COSM_BUILD_FOR}" MATCHES "ARGOS")
  if (SILICON_WITH_ROBOT_RAB)
    target_compile_definitions(${target} PUBLIC SILICON_WITH_ROBOT_RAB)
  endif()
  if (SILICON_WITH_ROBOT_BATTERY)
    target_compile_definitions(${target} PUBLIC SILICON_WITH_ROBOT_BATTERY)
  endif()
  if (SILICON_WITH_ROBOT_CAMERA)
    target_compile_definitions(${target} PUBLIC SILICON_WITH_ROBOT_CAMERA)
  endif()
  if (SILICON_WITH_ROBOT_LEDS)
    target_compile_definitions(${target} PUBLIC SILICON_WITH_ROBOT_LEDS)
  endif()
endif()


if ("${COSM_BUILD_FOR}" MATCHES "MSI")
  target_compile_options(${target} PUBLIC
    -Wno-missing-include-dirs
    -fno-new-inheriting-ctors)
endif()

################################################################################
# Exports                                                                      #
################################################################################
if (NOT IS_ROOT_PROJECT)
  set(${target}_INCLUDE_DIRS ${${target}_INCLUDE_DIRS} PARENT_SCOPE)
  set(${target}_LIBRARY_DIRS ${${target}_LIBRARY_DIRS} PARENT_SCOPE)
  set(${target}_LIBRARIES ${${target}_LIBRARIES} PARENT_SCOPE)
endif()
