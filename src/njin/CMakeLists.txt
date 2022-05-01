file(GLOB_RECURSE NJIN_HEADERS *.h)
file(GLOB_RECURSE NJIN_SOURCES *.cpp)

set(NJIN_NAME ${PROJECT_NAME})
set(NJIN_VERSION ${PROJECT_VERSION})
set(NJIN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(NJIN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(NJIN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(NJIN_SHADER_FOLDER shader)
else()
  set(NJIN_SHADER_FOLDER ${CMAKE_SOURCE_DIR}/shader)
endif()
configure_file(config.h.in config.h)

add_library(njin "${NJIN_HEADERS}" "${NJIN_SOURCES}")
target_include_directories(njin PUBLIC ${CMAKE_SOURCE_DIR}/src/njin
                                        ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
	njin PUBLIC njin_options CONAN_PKG::glad CONAN_PKG::glfw
               CONAN_PKG::glm)
