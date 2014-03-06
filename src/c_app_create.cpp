#include <iostream>
#include "c_application.hpp"
#include "c_logger.hpp"

using namespace gg;

/**
 * When gglib is compiled as a dynamic library, std::cout has its own
 * instance inside the library, therefore the one in the process space
 * won't be affected by logger.
 *
 * As a workaround, this compilation unit is excluded from the linking
 * and added to the interface library (gglib.a) manually by these post
 * build steps:
 *
 *  $compiler $options $includes -c src\c_app_create.cpp -o obj\release\src\c_app_create.o
 *  ar.exe -r -s bin\libgglib.a obj\release\src\c_app_create.o
 */

static void inject_cout()
{
    c_logger::get_instance()->register_cout(std::cout);
}

application* application::create(std::string name)
{
    c_application::set_init_callback(inject_cout);
    return new c_application(name);
}
