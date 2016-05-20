set( lib_name ${COMMON_LIB_NAME} )

list(APPEND src_dirs .)

foreach( src_dir ${src_dirs} )
    aux_source_directory(${src_dir} lib_src)
    file(GLOB headers ${src_dir}/*.h)
    list(APPEND lib_src ${headers})
endforeach( )

#include_directories( ${SRPC_INCLUDE_DIR}/vtrc-client )

add_library(${lib_name} ${LINK_TYPE} ${lib_src})


include_directories( ${INCLUDE_DIR}/common )

####################### PROTOCOL BASE #######################

set( ${lib_name}_POTOBUF_IMPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protocol-base )

file(GLOB protofiles protocol-base/*.proto)

set( ${lib_name}_PROTOBUF_FILES_DIR    ${CMAKE_CURRENT_BINARY_DIR} )

set( ${lib_name}_PROTOBUF_FILES_OUTPUT
     ${${lib_name}_PROTOBUF_FILES_DIR} )

file( MAKE_DIRECTORY ${${lib_name}_PROTOBUF_FILES_OUTPUT})

include_directories( ${${lib_name}_PROTOBUF_FILES_DIR} )

foreach(next_proto ${protofiles})

    get_filename_component( out_FILE ${next_proto} NAME_WE )
    get_filename_component( in_PATH  ${next_proto} PATH )

    set( OUTPUT_FILES
             ${${lib_name}_PROTOBUF_FILES_OUTPUT}/${out_FILE}.pb.h
             ${${lib_name}_PROTOBUF_FILES_OUTPUT}/${out_FILE}.pb.cc
             ${VTRC_PROTOCOL_INCLUDE_DIR}/${out_FILE}.pb.h
       )

    add_custom_command(
        COMMAND
                ${PROTOBUF_PROTOC_EXECUTABLE}
                        --proto_path=${PROTOBUF_INCLUDE_DIRS}
                        --proto_path=${POTOBUF_IMPORT_DIRS}
                        --proto_path=${in_PATH}
                        --cpp_out=${${lib_name}_PROTOBUF_FILES_OUTPUT}
                        ${next_proto}

        COMMAND
                ${CMAKE_COMMAND} -E copy
                    ${${lib_name}_PROTOBUF_FILES_OUTPUT}/${out_FILE}.pb.h
                    ${VTRC_PROTOCOL_INCLUDE_DIR}/${out_FILE}.pb.h

        WORKING_DIRECTORY
                ${GENERATED_FILES_OUTPUT}
        OUTPUT
                ${OUTPUT_FILES}
        DEPENDS
                ${next_proto}
        )

    set_source_files_properties(
        ${OUTPUT_FILES}
        PROPERTIES
        GENERATED TRUE
    )

    list( APPEND lib_src ${OUTPUT_FILES})
    list( APPEND lib_src ${next_proto})

endforeach( )

list( APPEND public_proto_files
        #vtrc-rpc-options.proto
        #vtrc-errors.proto
    )

foreach( next_proto ${public_proto_files} )

    file(COPY
            protocol-base/${next_proto}
        DESTINATION
            ${PROTOCOL_INCLUDE_DIR} )

endforeach( )

##################################################
