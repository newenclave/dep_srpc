#include <iostream>

#include "srpc/common/config/memory.h"
#include "srpc/common/transport/delegates/message.h"
#include "srpc/common/sizepack/varint.h"
#include "srpc/common/sizepack/fixint.h"

using namespace srpc;

using size_policy = common::sizepack::varint<size_t>;

struct keeper: public srpc::enable_shared_from_this<keeper> {

};

int main( int argc, char *argv[] )
{

}
