# This CMake script tries to find an AFL++ installation on the local machine. If no AFL++
# installations could be found, then this script reports a fatal error and stops configuring.

set(AFLPP_DIR CACHE STRING "Specify the root directory of AFL++")
if(NOT EXISTS "${AFLPP_DIR}" OR NOT IS_DIRECTORY "${AFLPP_DIR}")
    message(FATAL_ERROR "Could not find a usable AFL++ installation at \"${AFLPP_DIR}\"")
endif()

# TODO: Finish me.
