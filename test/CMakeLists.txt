if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

set(INTERNAL_PROJECT_NAME test)

project(${INTERNAL_PROJECT_NAME})

find_package( Protobuf REQUIRED)

include_directories( ${INCLUDE_DIRS} )

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

