
if(WIN32)
    set(
        CMAKE_USER_MAKE_RULES_OVERRIDE
        "${CMAKE_CURRENT_SOURCE_DIR}/msvc_override.cmake"
    )
endif(WIN32)

cmake_minimum_required(VERSION 2.8)

project(srpc)

include( cxx11_check.cmake )

set( CXX11_ENABLED 0 )

if( NOT DISABLE_CXX11 )
    check_cxx11( CXX11_ENABLED )
endif( )

find_package( Protobuf REQUIRED)

set( COMMON_LIB_NAME srpc_common )
set( SERVER_LIB_NAME srpc_server )
set( CLIENT_LIB_NAME srpc_client )


add_subdirectory( common )
add_subdirectory( server )
add_subdirectory( client )

