
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

add_definitions( -DCXX11_ENABLED=${CXX11_ENABLED} )
add_definitions( -DCONFIG_TOPNAMESPACE=srpc )

get_cmd_cxx11( CXX_CMD )
get_compiler_thread_local( THREAD_LOCAL )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_CMD}")

set( LINK_TYPE_STATIC STATIC )
set( LINK_TYPE_SHARED SHARED )
set( LINK_TYPE_MODULE MODULE )

if( NOT DEFINED LINK_TYPE )
    set( LINK_TYPE STATIC )
endif( )

if( CXX11_ENABLED )
    if( ASIO_STANDALONE_DIR )
        include_directories( ${ASIO_STANDALONE_DIR} )
        add_definitions( -DASIO_STANALONE=1 )
    else( )
        find_package( Boost 1.53 COMPONENTS
                            system
                        REQUIRED)
    endif( ASIO_STANDALONE_DIR )
else( CXX11_ENABLED )
    find_package( Boost 1.53 COMPONENTS
                        system
                        chrono
                        date_time
                        thread
                    REQUIRED)
endif( CXX11_ENABLED )

find_package( Protobuf REQUIRED)

set( INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include )

if( NOT DEFINED PROTOCOL_DIR )
    set( PROTOCOL_INCLUDE_DIR ${INCLUDE_DIR}/srpc/common/protocol )
else( )
    set( PROTOCOL_INCLUDE_DIR ${PROTOCOL_DIR} )
endif( )

set( INCLUDE_DIRS
     ${INCLUDE_DIR}
     ${PROTOCOL_INCLUDE_DIR}
     ${PROTOBUF_INCLUDE_DIRS}
     ${Boost_INCLUDE_DIRS}
   )

include_directories( ${PROJECT_SOURCE_DIR} ${INCLUDE_DIRS} )

set( COMMON_LIB_NAME srpc_common )
set( SERVER_LIB_NAME srpc_server )
set( CLIENT_LIB_NAME srpc_client )

add_subdirectory( lib/common )
add_subdirectory( lib/server )
add_subdirectory( lib/client )

add_subdirectory( test )


