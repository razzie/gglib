#ifndef MANAGED_COUT_HPP_INCLUDED
#define MANAGED_COUT_HPP_INCLUDED

#include <stack>
#include "gg/misc.hpp"

namespace gg
{
    class managed_cout
    {
        static void push_hook(std::ostream&);
        static void pop_hook();

    public:
        managed_cout() = delete;
        ~managed_cout() = delete;

        static void enable();
        static void disable();
        static bool is_enabled();

        class hook
        {
        public:
            hook(std::ostream& o) { managed_cout::push_hook(o); }
            hook(const hook&) = delete;
            hook(hook&&) = delete;
            ~hook() { managed_cout::pop_hook(); }
        };
    };
};

#endif // MANAGED_COUT_HPP_INCLUDED
