# Find GLEW library and include paths for CMU462
# This defines the following:
#
# GLEW_FOUND             If GLEW is found
# GLEW_LIBRARIES         GLEW libraries
# GLEW_INCLUDE_DIRS      GLEW include directories
# GLEW_LIBRARY_DIRS      GLEW library directories

if(UNIX)
  set(GLEW_INC_NAMES glew.h)
  set(GLEW_LIB_NAMES libglew.a)
  if(APPLE)
    set(GLEW_LIB_NAMES libglew_osx.a)
  endif(APPLE)
endif(UNIX)

# GLEW static library
find_library(GLEW_LIBRARIES
    NAMES ${GLEW_LIB_NAMES}
    PATHS ${PROJECT_SOURCE_DIR}/../lib
    DOC "GLEW library")

# GLEW library dir
find_path(GLEW_LIBRARY_DIRS
    NAMES ${GLEW_LIB_NAMES}
    PATHS ${PROJECT_SOURCE_DIR}/../lib
    DOC "462 include directories")

# GLEW include dir
find_path(GLEW_INCLUDE_DIRS
    NAMES ${GLEW_INC_NAMES}
    PATHS ${PROJECT_SOURCE_DIR}/../include/GLEW
    DOC "462 include directories")

# Version
set(GLEW_VERSION 1.13.0)

# Set package standard args
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEW 
    REQUIRED_VARS GLEW_LIBRARIES GLEW_INCLUDE_DIRS GLEW_LIBRARY_DIRS
    VERSION_VAR GLEW_VERSION)
