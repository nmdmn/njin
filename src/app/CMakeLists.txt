file(GLOB_RECURSE APP_HEADERS *.h)
file(GLOB_RECURSE APP_SOURCES *.cpp)

add_executable(app "${APP_HEADERS}" "${APP_SOURCES}")
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/src
                                        ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(app PRIVATE njin)
