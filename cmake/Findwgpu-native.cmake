find_library(wgpu-native_LIBRARIES NAMES "wgpu_native" HINTS "${WGPU_NATIVE_ROOT}")
find_path(wgpu-native_INCLUDE_DIRS NAMES "webgpu.h" HINTS "${WGPU_NATIVE_ROOT}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(wgpu-native DEFAULT_MSG wgpu-native_INCLUDE_DIRS wgpu-native_LIBRARIES)

if(wgpu-native_FOUND AND NOT TARGET wgpu-native)
	add_library(wgpu-native SHARED IMPORTED)
	set_target_properties(wgpu-native PROPERTIES
		IMPORTED_LOCATION "${wgpu-native_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${wgpu-native_INCLUDE_DIRS}"
	)
endif()

mark_as_advanced(wgpu-native_INCLUDE_DIRS wgpu-native_LIBRARIES)

