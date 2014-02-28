#include "c_application.hpp"
#include "c_logger.hpp"

using namespace gg;

class dllhack_init
{
    static void inject_cout()
    {
        c_logger::get_instance()->register_cout(std::cout);
    }

public:
    dllhack_init()
    {
        c_application::set_init_callback(&dllhack_init::inject_cout);
    }

    ~dllhack_init()
    {
    }
};

dllhack_init __dllhack;
