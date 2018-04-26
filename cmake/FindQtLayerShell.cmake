find_package(PkgConfig)

if (QTLAYERSHELL_FIND_REQUIRED)
	set(_pkgconfig_REQUIRED "REQUIRED")
else()
	set(_pkgconfig_REQUIRED "")
endif()

if(QTLAYERSHELL_FIND_VERSION)
    pkg_check_modules(PC_LAYERSHELL ${_pkgconfig_REQUIRED} qtlayershell=${QTLAYERSHELL_FIND_VERSION})
else()
    pkg_check_modules(PC_LAYERSHELL ${_pkgconfig_REQUIRED} qtlayershell)
endif()

find_path(QTLAYERSHELL_INCLUDE_DIRS NAMES QtLayerShell/layerview.h HINTS ${PC_LAYERSHELL_INCLUDE_DIRS})
find_library(QTLAYERSHELL_LIBRARIES NAMES qtlayershell HINTS ${PC_LAYERSHELL_LIBRARY_DIRS})
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(QTLAYERSHELL DEFAULT_MSG QTLAYERSHELL_LIBRARIES QTLAYERSHELL_INCLUDE_DIRS)
mark_as_advanced(QTLAYERSHELL_LIBRARIES QTLAYERSHELL_INCLUDE_DIRS)
