
macro( srpc_all_headers_path OUT_LIST PREFIX_PATH )
    list( APPEND ${OUT_LIST}
        ${PREFIX_PATH}/srpc/server
        ${PREFIX_PATH}/srpc/common
        ${PREFIX_PATH}/srpc/common/hash
        ${PREFIX_PATH}/srpc/common/timers
        ${PREFIX_PATH}/srpc/common/timers/calls
        ${PREFIX_PATH}/srpc/common/sizepack
        ${PREFIX_PATH}/srpc/common/observers
        ${PREFIX_PATH}/srpc/common/observers/traits
        ${PREFIX_PATH}/srpc/common/details
        ${PREFIX_PATH}/srpc/common/cache
        ${PREFIX_PATH}/srpc/common/cache/traits
        ${PREFIX_PATH}/srpc/common/result
        ${PREFIX_PATH}/srpc/common/result/traits
        ${PREFIX_PATH}/srpc/common/queues
        ${PREFIX_PATH}/srpc/common/protocol
        ${PREFIX_PATH}/srpc/common/protocol/default
        ${PREFIX_PATH}/srpc/common/protocol/traits
        ${PREFIX_PATH}/srpc/common/protobuf
        ${PREFIX_PATH}/srpc/common/protobuf/service
        ${PREFIX_PATH}/srpc/common/queues/traits
        ${PREFIX_PATH}/srpc/common/transport
        ${PREFIX_PATH}/srpc/common/transport/delegates
        ${PREFIX_PATH}/srpc/common/transport/async
        ${PREFIX_PATH}/srpc/common/config
        ${PREFIX_PATH}/srpc/server
        ${PREFIX_PATH}/srpc/server/acceptor
        ${PREFIX_PATH}/srpc/server/listener
        ${PREFIX_PATH}/srpc/server/acceptor/async
        ${PREFIX_PATH}/srpc/client
        ${PREFIX_PATH}/srpc/client/connector
        ${PREFIX_PATH}/srpc/client/connector/async
        ${PREFIX_PATH}/srpc/client/connector/async/impl
    )
endmacro( )
