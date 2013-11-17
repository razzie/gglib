#ifndef GG_CONSOLE_HPP_INCLUDED
#define GG_CONSOLE_HPP_INCLUDED

#include "gg/core.hpp"

namespace gg
{
    class console : public reference_counted
    {
    public:
        class output : public reference_counted, public virtual std::ostream
        {
        public:
            virtual ~output() {}
            virtual console& get_console() const = 0;
            virtual void show() = 0;
            virtual void hide() = 0;
            virtual void set_color(gg::color) = 0;
            virtual gg::color get_color() const = 0;
            virtual void align_left() = 0;
            virtual void align_center() = 0;
            virtual void align_right() = 0;
            virtual bool is_empty() const = 0;
            virtual std::string to_string() const = 0;
        };

        class controller : public reference_counted
        {
        public:
            enum class exec_result
            {
                EXEC_SUCCESS,
                EXEC_FAIL,
                NO_EXEC
            };

            virtual ~controller() {}
            virtual exec_result exec(std::string& cmd, output&) = 0;
            virtual void complete(std::string& cmd, output&) = 0;
        };

        virtual ~console() {};
        virtual void open() = 0;
        virtual void close() = 0;
        virtual bool is_opened() const = 0;
        virtual output* create_output() = 0;
        virtual void clear() = 0;
    };
};

#endif // GG_CONSOLE_HPP_INCLUDED
