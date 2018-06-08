# - Find rdma verbs
# Find the rdma verbs library and includes
#
# VERBS_INCLUDE_DIR - where to find ibverbs.h, etc.
# VERBS_LIBRARIES - List of libraries when using ibverbs.
# VERBS_FOUND - True if ibverbs found.

find_path(VERBS_INCLUDE_DIR infiniband/verbs.h)

set(VERBS_NAMES ${VERBS_NAMES} ibverbs)
find_library(VERBS_LIBRARY NAMES ${VERBS_NAMES})

if (VERBS_INCLUDE_DIR AND VERBS_LIBRARY)
  set(VERBS_FOUND TRUE)
  set(VERBS_LIBRARIES ${VERBS_LIBRARY})
else ()
  set(VERBS_FOUND FALSE)
  set( VERBS_LIBRARIES )
endif ()

if (VERBS_FOUND)
  message(STATUS "Found libibverbs: ${VERBS_LIBRARY}")

  include(CheckCXXSourceCompiles)
  CHECK_CXX_SOURCE_COMPILES("
    #include <infiniband/verbs.h>
    int main() {
      struct ibv_context* ctxt;
      struct ibv_exp_gid_attr gid_attr;
      ibv_exp_query_gid_attr(ctxt, 1, 0, &gid_attr);
      return 0;
    } " HAVE_IBV_EXP)

else ()
  message(STATUS "Not Found libibverbs: ${VERBS_LIBRARY}")
  if (VERBS_FIND_REQUIRED)
    message(STATUS "Looked for libibverbs named ${VERBS_NAMES}.")
    message(FATAL_ERROR "Could NOT find libibverbs")
  endif ()
endif ()

# handle the QUIETLY and REQUIRED arguments and set UUID_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ibverbs DEFAULT_MSG VERBS_LIBRARIES VERBS_INCLUDE_DIR)

mark_as_advanced(
  VERBS_LIBRARY
)
