
macro( all_headers_path OUT_LIST )
    list( APPEND OUT_LIST
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/server
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/hash
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/timers
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/timers/calls
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/sizepack
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/queues
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/queues/traits
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/transport
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/transport/delegates
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/transport/async
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/common/config
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/server
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/server/acceptor
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/server/acceptor/async
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/client
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/client/connector
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/client/connector/async
          ${CMAKE_CURRENT_SOURCE_DIR}/include/srpc/client/connector/async/impl
          )
endmacro( )
