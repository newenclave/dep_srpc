set( lib_name ${CLIENT_LIB_NAME} )

list(APPEND src_dirs .)

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

include_directories( ${SRPC_INCLUDE_DIR}/client )

add_library(${lib_name} ${LINK_TYPE} ${lib_src})


