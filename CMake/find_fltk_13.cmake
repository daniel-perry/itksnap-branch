# This script sets up FLTK 1.3 because FindFLTK does not work very well

# The name of the FLTK library will vary depending on the system
# Options are libfltk_forms.a fltkforms.lib and fltkformsd.lib
IF(MSVC)
  SET(PREF "")
  IF(CMAKE_BUILD_TYPE MATCHES "Debug")
    SET(SUFF "d")
  ELSE(CMAKE_BUILD_TYPE MATCHES "Debug")
    SET(SUFF "")
  ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")
ELSE(MSVC)
  SET(PREF "_")
  SET(SUFF "")
ENDIF(MSVC)

# What is the target name of the fltk library
SET(BASETARG "fltk${SUFF}")

# Search for the base library
FIND_LIBRARY(FLTK_BASE_LIBRARY NAMES ${BASETARG} PATHS "/usr/lib" "/usr/local/lib")

IF(FLTK_BASE_LIBRARY)
  MESSAGE(STATUS "FLTK base: ${FLTK_BASE_LIBRARY}")

  # Search for the other libraries
  GET_FILENAME_COMPONENT(FLTK_LIB_DIR ${FLTK_BASE_LIBRARY} PATH)
  MESSAGE(STATUS "FLTK lib dir: ${FLTK_LIB_DIR}")
  MESSAGE(STATUS "FLTK target: fltk${PREF}forms${SUFF}")
  FIND_LIBRARY(FLTK_FORMS_LIBRARY NAMES "fltk${PREF}forms${SUFF}" PATHS ${FLTK_LIB_DIR})
  FIND_LIBRARY(FLTK_GL_LIBRARY NAMES "fltk${PREF}gl${SUFF}" PATHS ${FLTK_LIB_DIR})
  FIND_LIBRARY(FLTK_IMAGES_LIBRARY NAMES "fltk${PREF}images${SUFF}" PATHS ${FLTK_LIB_DIR})

  # Search for the fluid executable. It can be in a couple of places.
  # 1. In the bin directory, same level as lib
  # 2. In the fluid directiry, same level as lib (if built 'in-place')
  GET_FILENAME_COMPONENT(FLTK_ROOT_DIR ${FLTK_LIB_DIR} PATH)
  FIND_PROGRAM(FLTK_FLUID_EXECUTABLE 
    NAMES "fluid${SUFF}"
    PATHS "${FLTK_ROOT_DIR}/bin" "${FLTK_ROOT_DIR}/fluid")

  # Search for the FLTK include directory. Again, two places to look
  # 1. In the include directory
  # 2. In the root directory
  FIND_PATH(FLTK_INCLUDE_DIR
    NAMES "FL/Fl.H"
    PATHS "${FLTK_ROOT_DIR}/include" "${FLTK_ROOT_DIR}")
  MESSAGE(STATUS "FLTK include directory: ${FLTK_INCLUDE_DIR}")

  # For now, we let the user decide about the additional image libraries
  # TODO: figure this out from fltk-config file
  OPTION(SNAP_USE_FLTK_PNG "Should we link with the FLTK png library?" OFF)
  OPTION(SNAP_USE_FLTK_JPEG "Should we link with the FLTK jpeg library?" OFF)
  OPTION(SNAP_USE_FLTK_ZLIB "Should we link with the FLTK zlib library?" OFF)

  # Look for these libs if specified
  IF(SNAP_USE_FLTK_PNG)
    FIND_LIBRARY(FLTK_PNG_LIBRARY NAMES "fltk${PREF}png${SUFF}" PATHS ${FLTK_LIB_DIR})
    SET(FLTK_IMAGES_LIBS ${FLTK_IMAGES_LIBS} ${FLTK_PNG_LIBRARY})
    IF(FLTK_PNG_LIBRARY)
      SET(FLTK_PNG_LIBRARY_OK ON)
    ELSE(FLTK_PNG_LIBRARY)
      SET(FLTK_PNG_LIBRARY_OK OFF)
    ENDIF(FLTK_PNG_LIBRARY)
  ELSE(SNAP_USE_FLTK_PNG)
    FIND_PACKAGE(PNG REQUIRED)
    IF(PNG_FOUND)
      SET(FLTK_PNG_LIBRARY_OK ON)
      SET(FLTK_IMAGES_LIBS ${FLTK_IMAGES_LIBS} ${PNG_LIBRARY})
    ELSE(PNG_FOUND)
      SET(FLTK_PNG_LIBRARY_OK OFF)
    ENDIF(PNG_FOUND)
  ENDIF(SNAP_USE_FLTK_PNG)

  IF(SNAP_USE_FLTK_JPEG)
    FIND_LIBRARY(FLTK_JPEG_LIBRARY NAMES "fltk${PREF}jpeg${SUFF}" PATHS ${FLTK_LIB_DIR})
    SET(FLTK_IMAGES_LIBS ${FLTK_IMAGES_LIBS} ${FLTK_JPEG_LIBRARY})
    IF(FLTK_JPEG_LIBRARY)
      SET(FLTK_JPEG_LIBRARY_OK ON)
    ELSE(FLTK_JPEG_LIBRARY)
      SET(FLTK_JPEG_LIBRARY_OK OFF)
    ENDIF(FLTK_JPEG_LIBRARY)
  ELSE(SNAP_USE_FLTK_JPEG)
    FIND_PACKAGE(JPEG REQUIRED)
    IF(JPEG_FOUND)
      SET(FLTK_JPEG_LIBRARY_OK ON)
      SET(FLTK_IMAGES_LIBS ${FLTK_IMAGES_LIBS} ${JPEG_LIBRARY})
    ELSE(JPEG_FOUND)
      SET(FLTK_JPEG_LIBRARY_OK OFF)
    ENDIF(JPEG_FOUND)
  ENDIF(SNAP_USE_FLTK_JPEG)

  IF(SNAP_USE_FLTK_ZLIB)
    FIND_LIBRARY(FLTK_ZLIB_LIBRARY NAMES "fltk${PREF}z${SUFF}" "zlib${SUFF}" PATHS ${FLTK_LIB_DIR})
    SET(FLTK_IMAGES_LIBS ${FLTK_IMAGES_LIBS} ${FLTK_ZLIB_LIBRARY})
    IF(FLTK_ZLIB_LIBRARY)
      SET(FLTK_ZLIB_LIBRARY_OK ON)
    ELSE(FLTK_ZLIB_LIBRARY)
      SET(FLTK_ZLIB_LIBRARY_OK OFF)
    ENDIF(FLTK_ZLIB_LIBRARY)
  ELSE(SNAP_USE_FLTK_ZLIB)
    FIND_PACKAGE(ZLIB REQUIRED)
    IF(ZLIB_FOUND)
      SET(FLTK_ZLIB_LIBRARY_OK ON)
      SET(FLTK_IMAGES_LIBS ${FLTK_IMAGES_LIBS} ${ZLIB_LIBRARY})
    ELSE(ZLIB_FOUND)
      SET(FLTK_ZLIB_LIBRARY_OK OFF)
    ENDIF(ZLIB_FOUND)
  ENDIF(SNAP_USE_FLTK_ZLIB)

ENDIF(FLTK_BASE_LIBRARY)

# Check if all components have been found
IF(FLTK_BASE_LIBRARY AND FLTK_FORMS_LIBRARY AND FLTK_GL_LIBRARY AND FLTK_IMAGES_LIBRARY
    AND FLTK_INCLUDE_DIR AND FLTK_FLUID_EXECUTABLE 
    AND FLTK_PNG_LIBRARY_OK AND FLTK_ZLIB_LIBRARY_OK AND FLTK_JPEG_LIBRARY_OK)
  SET(FLTK_FOUND ON)
ELSE(FLTK_BASE_LIBRARY AND FLTK_FORMS_LIBRARY AND FLTK_GL_LIBRARY AND FLTK_IMAGES_LIBRARY
    AND FLTK_INCLUDE_DIR AND FLTK_FLUID_EXECUTABLE 
    AND FLTK_PNG_LIBRARY_OK AND FLTK_ZLIB_LIBRARY_OK AND FLTK_JPEG_LIBRARY_OK)
  SET(FLTK_FOUND OFF)
ENDIF(FLTK_BASE_LIBRARY AND FLTK_FORMS_LIBRARY AND FLTK_GL_LIBRARY AND FLTK_IMAGES_LIBRARY
    AND FLTK_INCLUDE_DIR AND FLTK_FLUID_EXECUTABLE 
    AND FLTK_PNG_LIBRARY_OK AND FLTK_ZLIB_LIBRARY_OK AND FLTK_JPEG_LIBRARY_OK)

# Taken from findfltk.cmake
#  Platform dependent libraries required by FLTK
IF(WIN32)
  IF(NOT CYGWIN)
    IF(BORLAND)
      SET( FLTK_PLATFORM_DEPENDENT_LIBS import32 )
    ELSE(BORLAND)
      SET( FLTK_PLATFORM_DEPENDENT_LIBS wsock32 comctl32 )
    ENDIF(BORLAND)
  ENDIF(NOT CYGWIN)
ENDIF(WIN32)

IF(UNIX)
  INCLUDE(${CMAKE_ROOT}/Modules/FindX11.cmake)
  FIND_LIBRARY(FLTK_MATH_LIBRARY m)
  SET( FLTK_PLATFORM_DEPENDENT_LIBS ${X11_LIBRARIES} ${X11_Xft_LIB} ${X11_Xinerama_LIB} ${FLTK_MATH_LIBRARY})
ENDIF(UNIX)

IF(APPLE)
  SET( FLTK_PLATFORM_DEPENDENT_LIBS  "-framework Carbon -framework Cocoa -framework ApplicationServices -framework AudioToolbox -lz")
ENDIF(APPLE)

IF(CYGWIN)
  FIND_LIBRARY(FLTK_MATH_LIBRARY m)
  SET( FLTK_PLATFORM_DEPENDENT_LIBS ole32 uuid comctl32 wsock32 supc++ ${FLTK_MATH_LIBRARY} -lgdi32)
ENDIF(CYGWIN)

# Set the FLTK libraries
SET(FLTK_LIBRARIES
  ${FLTK_BASE_LIBRARY} ${FLTK_FORMS_LIBRARY} ${FLTK_IMAGES_LIBRARY} ${FLTK_GL_LIBRARY}
  ${FLTK_IMAGES_LIBS} ${FLTK_PLATFORM_DEPENDENT_LIBS})

SET(FLTK_INCLUDE_PATH ${FLTK_INCLUDE_DIR})
