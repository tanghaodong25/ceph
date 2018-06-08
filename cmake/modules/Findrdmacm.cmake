# - Find rdma cm
# Find the rdma cm library and includes
#
# RDMACM_INCLUDE_DIR - where to find cma.h, etc.
# RDMACM_LIBRARIES - List of libraries when using rdmacm.
# RDMACM_FOUND - True if rdmacm found.

find_path(RDMACM_INCLUDE_DIR rdma/rdma_cma.h)

set(RDMACM_NAMES ${RDMACM_NAMES} rdmacm)
find_library(RDMACM_LIBRARY NAMES ${RDMACM_NAMES})

if (RDMACM_INCLUDE_DIR AND RDMACM_LIBRARY)
  set(RDMACM_FOUND TRUE)
  set(RDMACM_LIBRARIES ${RDMACM_LIBRARY})
else ()
  set(RDMACM_FOUND FALSE)
  set( RDMACM_LIBRARIES )
endif ()

if (RDMACM_FOUND)
  message(STATUS "Found librdmacm: ${RDMACM_LIBRARY}")

else ()
  message(STATUS "Not Found librdmacm: ${RDMACM_LIBRARY}")
  if (RDMACM_FIND_REQUIRED)
    message(STATUS "Looked for librdmacm named ${RDMACM_NAMES}.")
    message(FATAL_ERROR "Could NOT find librdmacm")
  endif ()
endif ()

# handle the QUIETLY and REQUIRED arguments and set UUID_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(rdmacm DEFAULT_MSG RDMACM_LIBRARIES RDMACM_INCLUDE_DIR)

mark_as_advanced(
  RDMACM_LIBRARY
)
