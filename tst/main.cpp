#include <iostream>
#include <string>
#include <vector>

#include "boost/asio.hpp"

#include "boost/program_options.hpp"
#include "options.h"


namespace po    = boost::program_options;
namespace asio  = boost::asio;

namespace tunnel {
    namespace server { int start( options &opts ); }
    namespace client { int start( options &opts ); }
}

namespace {

    tunnel::options g_opts;

    void fill_all_options( po::options_description &desc )
    {
        using string_list = std::vector<std::string>;
        desc.add_options( )
                ("help,h",      "help message")
                ("server,s",    "run as server side")
                ("client,c",    "run as client side")
                ("tap",         "Use as tap device")
                ("udp",         "Use UDP transport")

                ("dev,d",   po::value(&g_opts.device),  "use the device")
                ("addr,a",  po::value(&g_opts.address), "ip address")
                ("port,p",  po::value(&g_opts.port),    "port")

        ;
    }

    auto create_cmd_params( int argc, const char **argv,
                            po::options_description const &desc )
    {
        po::variables_map vm;
        po::parsed_options parsed (
            po::command_line_parser(argc, argv)
                .options(desc)
                .allow_unregistered( )
                .run( ));
        po::store( parsed, vm );
        po::notify( vm);
        return vm;
    }
}

int main( int argc, const char **argv )
{

    try {

        po::options_description options("Available options");
        fill_all_options( options );
        auto opts = create_cmd_params( argc, argv, options );
        if( opts.count("help") ) {
            std::cout << options << "\n";
            return 0;
        }

        g_opts.use_tap = !!opts.count("tap");
        g_opts.use_udp = !!opts.count("udp");

        if( opts.count("client") ) {

            return tunnel::client::start(g_opts);

        } else if( opts.count("server") ) {

            return tunnel::server::start(g_opts);

        } else {
            std::cerr << "unknown run mode; do specify -c or -s\n";
            std::cerr << options << "\n";
            return 2;
        }

    } catch( const std::exception &ex ) {
        std::cerr << "Error: " << ex.what( ) << "\n";
        return 1;
    }

    return 0;
}
