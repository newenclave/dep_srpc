
aux_source_directory(. src_dir)

set( exe_name ${INTERNAL_PROJECT_NAME}_server )

add_executable(${exe_name} ${src_dir})

list( APPEND internal
      ${INTERNAL_PROJECT_NAME}_common_s
      srpc_common
      srpc_server
      Boost
)

include_directories( ${${INTERNAL_PROJECT_NAME}_common_s_INCLUDE_DIRS}  )

message( "-- Seting up internal dependencies for " ${exe_name} )
foreach( dep ${internal} )

    message( "\tAdd dependence: " ${dep} )
    message( "\tSERVER LIBRARY!: " ${${dep}_LIBRARIES} )

    add_dependencies(       ${exe_name} ${dep}      )
    add_definitions(        ${${dep}_DEFINITIONS}   )
    target_link_libraries(  ${exe_name} ${${dep}_LIBRARIES} )
    include_directories(    ${${dep}_INCLUDE_DIRS}  )

endforeach(dep)

if(WIN32)
    target_link_libraries( ${exe_name} ws2_32.lib )
endif(WIN32)

if(MINGW)
    target_link_libraries( ${exe_name} ws2_32 mswsock )
endif(MINGW)

set_target_properties( ${exe_name}
                       PROPERTIES COMPILE_DEFINITIONS "${LINK_TYPE}" )

