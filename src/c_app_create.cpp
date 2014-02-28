#include "c_application.hpp"

using namespace gg;

application* application::create(std::string name)
{
    return new c_application(name);
}
