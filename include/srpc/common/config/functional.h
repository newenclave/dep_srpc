#ifndef CONFIG_FUNCTIONAL_H
#define CONFIG_FUNCTIONAL_H


#if CXX11_ENABLED == 0

#include "boost/function.hpp"
#include "boost/bind.hpp"

namespace CONFIG_TOPNAMESPACE {
    using boost::function;
    using boost::bind;
    using boost::ref;
    using boost::cref;

    namespace placeholders {
        namespace {
            boost::arg<1> _1;
            boost::arg<2> _2;
            boost::arg<3> _3;
            boost::arg<4> _4;
            boost::arg<5> _5;
            boost::arg<6> _6;
            boost::arg<7> _7;
            boost::arg<8> _8;
            boost::arg<9> _9;
        }

        static boost::arg<1> &error             = _1;
        static boost::arg<2> &bytes_transferred = _2;
    //        static boost::arg<2> &iterator          = _2;
    //        static boost::arg<2> &signal_number     = _2;
    }
}

#else

#include <functional>

namespace CONFIG_TOPNAMESPACE {
    using std::function;
    using std::bind;
    using std::ref;
    using std::cref;

    namespace placeholders {

        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;
        using std::placeholders::_7;
        using std::placeholders::_8;
        using std::placeholders::_9;

        static decltype( _1 ) &error             = _1;
        static decltype( _2 ) &bytes_transferred = _2;

    }
}

#endif

#endif // VTRCFUNCTION_H
