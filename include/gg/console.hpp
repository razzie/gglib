#ifndef GG_CONSOLE_HPP_INCLUDED
#define GG_CONSOLE_HPP_INCLUDED

#include <string>
#include <sstream>
#include "gg/refcounted.hpp"
#include "gg/var.hpp"

namespace gg
{
    class console : public reference_counted
    {
    public:
        class output : public reference_counted
        {
        public:
            virtual ~output() {}
            virtual console& get_console() const = 0;
            virtual void show() = 0;
            virtual void hide() = 0;
            virtual void set_color(gg::color) = 0;
            virtual gg::color get_color() const = 0;
            virtual void align_left() = 0;
            virtual void align_right() = 0;
            virtual bool is_empty() const = 0;
            virtual output& operator<< (const gg::var&) = 0;
            virtual std::string get_string() const = 0;
        };

        class controller : public reference_counted
        {
        public:
            virtual ~controller() {}
            virtual bool exec(std::string& cmd, output&) = 0;
            virtual void complete(std::string& cmd, output&) = 0;
        };

        virtual ~console() {};
        virtual void open() = 0;
        virtual void close() = 0;
        virtual bool is_opened() const = 0;
        virtual output* create_output() = 0;
        virtual void clear() = 0;

        static console* get_invoker();
    };
};

#endif // GG_CONSOLE_HPP_INCLUDED
