#include "gg/var.hpp"

using namespace gg;

std::ostream& operator<< (std::ostream& o, const gg::varlist& vl)
{
    int i;
    int size = vl.size();

    o << "[";

    for (i=0; i<size; ++i)
        o << vl[i].to_stream() << (i<size-1 ? ", " : "");

    o << "]";

    return o;
}
