set(NDI_SDK_INCLUDE_PATH "" CACHE PATH "NDI SDK include path")
set(NDI_SDK_LIBRARY_PATH "" CACHE PATH "NDI SDK library path")

if(NOT (NDI_SDK_INCLUDE_PATH AND NDI_SDK_LIBRARY_PATH))
  message(FATAL_ERROR "NDI SDK: Please provide NDI_SDK_INCLUDE_PATH and NDI_SDK_LIBRARY_PATH!")
endif()

find_path(NDI_INCLUDE_DIR
  NAMES
    Processing.NDI.compat.h
    Processing.NDI.deprecated.h
    Processing.NDI.DynamicLoad.h
    Processing.NDI.Find.h
    Processing.NDI.FrameSync.h
    Processing.NDI.Lib.cplusplus.h
    Processing.NDI.Lib.h
    Processing.NDI.Recv.ex.h
    Processing.NDI.Recv.h
    Processing.NDI.Routing.h
    Processing.NDI.Send.h
    Processing.NDI.structs.h
    Processing.NDI.utilities.h
  PATHS "${NDI_SDK_INCLUDE_PATH}"
)

find_library(NDI_LIBRARY
  NAMES ndi
  PATHS "${NDI_SDK_LIBRARY_PATH}"
)
if(NOT NDI_LIBRARY)
  message(FATAL_ERROR "NDI SDK: libndi.so / ndi.dll not found in:\n${NDI_SDK_LIBRARY_PATH}\nMaybe you have to create a symlink or rename the file.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NDI
  FOUND_VAR NDI_FOUND
  REQUIRED_VARS
    NDI_LIBRARY
    NDI_INCLUDE_DIR
)

if(NDI_FOUND AND NOT TARGET NDI::NDI)
  add_library(NDI::NDI SHARED IMPORTED)
  set_target_properties(NDI::NDI PROPERTIES
    IMPORTED_LOCATION "${NDI_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${NDI_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  NDI_INCLUDE_DIR
  NDI_LIBRARY
)
