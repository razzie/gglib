#ifndef MANAGED_COUT_HPP_INCLUDED
#define MANAGED_COUT_HPP_INCLUDED

#include <map>
#include <vector>
#include "tinythread.h"
#include "gg/types.hpp"
#include "gg/console.hpp"

namespace gg
{
    class managed_cout
    {
        class managed_buf : public std::streambuf
        {
        public:
            managed_buf();
            virtual ~managed_buf();
        protected:
            virtual int overflow (int c);
        };

        struct callback
        {
            enum { STREAM, CONSOLE } type;
            union
            {
                std::ostream* stream;
                gg::console::output* console;
            } data;
        };

        tthread::mutex m_mutex;
        std::streambuf* m_cout_rdbuf = nullptr;
        managed_buf* m_own_rdbuf = nullptr;
        std::map<tthread::thread::id, std::vector<callback>> m_hooks;

    protected:
        managed_cout();
        ~managed_cout();
        void push_hook(std::ostream&);
        void push_hook(console::output&);
        void pop_hook();

    public:
        static managed_cout* get_instance();

        void enable();
        void disable();
        bool is_enabled() const;

        class hook
        {
        public:
            hook(std::ostream& o) { managed_cout::get_instance()->push_hook(o); }
            hook(console::output& o) { managed_cout::get_instance()->push_hook(o); }
            hook(const hook&) = delete;
            hook(hook&&) = delete;
            ~hook() { managed_cout::get_instance()->pop_hook(); }
        };
    };
};

#endif // MANAGED_COUT_HPP_INCLUDED
