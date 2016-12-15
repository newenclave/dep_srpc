
macro( srpc_all_headers_path OUT_LIST PREFIX_PATH )
    list( APPEND ${OUT_LIST}
          ${PREFIX_PATH}/include/srpc/server
          ${PREFIX_PATH}/include/srpc/common
          ${PREFIX_PATH}/include/srpc/common/hash
          ${PREFIX_PATH}/include/srpc/common/timers
          ${PREFIX_PATH}/include/srpc/common/timers/calls
          ${PREFIX_PATH}/include/srpc/common/sizepack
          ${PREFIX_PATH}/include/srpc/common/queues
          ${PREFIX_PATH}/include/srpc/common/queues/traits
          ${PREFIX_PATH}/include/srpc/common/transport
          ${PREFIX_PATH}/include/srpc/common/transport/delegates
          ${PREFIX_PATH}/include/srpc/common/transport/async
          ${PREFIX_PATH}/include/srpc/common/config
          ${PREFIX_PATH}/include/srpc/server
          ${PREFIX_PATH}/include/srpc/server/acceptor
          ${PREFIX_PATH}/include/srpc/server/acceptor/async
          ${PREFIX_PATH}/include/srpc/client
          ${PREFIX_PATH}/include/srpc/client/connector
          ${PREFIX_PATH}/include/srpc/client/connector/async
          ${PREFIX_PATH}/include/srpc/client/connector/async/impl
          )
endmacro( )
