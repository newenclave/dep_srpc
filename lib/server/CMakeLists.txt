set( lib_name ${SERVER_LIB_NAME} )

list( APPEND src_dirs
      .
      ${INCLUDE_DIR}/srpc/server
      ./listener
     )

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

include_directories( ${SRPC_INCLUDE_DIR}/server )

add_library(${lib_name} ${LINK_TYPE} ${lib_src})

get_target_property(${lib_name}_LOCATION ${lib_name} LOCATION)

set( ${lib_name}_LIBRARY ${${lib_name}_LOCATION} PARENT_SCOPE)

set( ${lib_name}_LIBRARIES
     ${${lib_name}_LOCATION}
     PARENT_SCOPE )

