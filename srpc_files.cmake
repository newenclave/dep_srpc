
macro( srpc_all_headers_path OUT_LIST ADD_PATH )
    list( APPEND ${OUT_LIST}
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/server
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/hash
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/timers
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/timers/calls
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/sizepack
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/queues
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/queues/traits
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/transport
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/transport/delegates
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/transport/async
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/common/config
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/server
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/server/acceptor
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/server/acceptor/async
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/client
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/client/connector
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/client/connector/async
          ${CMAKE_CURRENT_SOURCE_DIR}/${ADD_PATH}/include/srpc/client/connector/async/impl
          )
endmacro( )
