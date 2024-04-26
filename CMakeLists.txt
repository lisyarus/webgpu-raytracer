cmake_minimum_required(VERSION 3.2)
project(webgpu-raytracer)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)
find_package(wgpu-native REQUIRED)

file(GLOB_RECURSE WEBGPU_RAYTRACER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/source/*")
file(GLOB_RECURSE WEBGPU_RAYTRACER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

add_executable(webgpu-raytracer
	${WEBGPU_RAYTRACER_HEADERS}
	${WEBGPU_RAYTRACER_SOURCES}
)

target_link_libraries(webgpu-raytracer
	SDL2::SDL2
	wgpu-native
)

target_include_directories(webgpu-raytracer PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/glm"
	"${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/stb"
)

target_compile_definitions(webgpu-raytracer PUBLIC
	-DPROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(APPLE)
	set_source_files_properties("source/sdl_wgpu.c" PROPERTIES COMPILE_FLAGS "-x objective-c")
	target_link_libraries(webgpu-raytracer
		"-framework QuartzCore"
		"-framework Cocoa"
		"-framework Metal"
	)
endif()
