
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

set( LINK_TYPE_STATIC STATIC )
set( LINK_TYPE_SHARED SHARED )
set( LINK_TYPE_MODULE MODULE )

if( NOT DEFINED LINK_TYPE )
    set( LINK_TYPE STATIC )
endif( )

find_package( Protobuf REQUIRED)

set( INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include )

if( NOT DEFINED PROTOCOL_DIR )
    set( PROTOCOL_INCLUDE_DIR ${INCLUDE_DIR}/common/protocol )
else( )
    set( PROTOCOL_INCLUDE_DIR ${PROTOCOL_DIR} )
endif( )

set( INCLUDE_DIRS
     ${INCLUDE_DIR}
     ${PROTOCOL_INCLUDE_DIR}
     ${PROTOBUF_INCLUDE_DIRS}
   )

include_directories( ${PROJECT_SOURCE_DIR} ${INCLUDE_DIRS} )

set( COMMON_LIB_NAME srpc_common )
set( SERVER_LIB_NAME srpc_server )
set( CLIENT_LIB_NAME srpc_client )

add_subdirectory( common )
add_subdirectory( server )
add_subdirectory( client )

add_subdirectory( test )


