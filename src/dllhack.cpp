#include <iostream>
#include "c_application.hpp"
#include "c_logger.hpp"

using namespace gg;


static void inject_cout()
{
    c_logger::get_instance()->register_cout(std::cout);
}

application* application::create(std::string name)
{
    c_application::set_init_callback(inject_cout);
    return new c_application(name);
}
