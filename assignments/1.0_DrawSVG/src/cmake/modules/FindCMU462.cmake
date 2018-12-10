# Find CMU462 library and include paths
# This defines the following:
#
# CMU462_FOUND             If CMU462 is found
# CMU462_LIBRARIES         CMU462 libraries
# CMU462_INCLUDE_DIRS      CMU462 include directories
# CMU462_LIBRARY_DIRS      CMU462 library directories

if(UNIX)
  set(CMU462_INC_NAMES CMU462.h)
  set(CMU462_LIB_NAMES libCMU462.a)
  if(APPLE)
    set(CMU462_LIB_NAMES libCMU462_osx.a)
  endif(APPLE)
endif(UNIX)

# CMU462 static library
find_library(CMU462_LIBRARIES
    NAMES ${CMU462_LIB_NAMES}
    PATHS ${PROJECT_SOURCE_DIR}/../lib
    DOC "CMU462 library")

# CMU462 library dir
find_path(CMU462_LIBRARY_DIRS
    NAMES ${CMU462_LIB_NAMES}
    PATHS ${PROJECT_SOURCE_DIR}/../lib
    DOC "462 include directories")

# CMU462 include dir
find_path(CMU462_INCLUDE_DIRS
    NAMES ${CMU462_INC_NAMES}
    PATHS ${PROJECT_SOURCE_DIR}/../include/
    DOC "462 include directories")

# Version
set(CMU462_VERSION 1.0)

# Set package standard args
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CMU462 
    REQUIRED_VARS CMU462_INCLUDE_DIRS CMU462_LIBRARY_DIRS CMU462_LIBRARIES
    VERSION_VAR CMU462_VERSION)
