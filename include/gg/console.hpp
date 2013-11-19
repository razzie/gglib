#ifndef GG_CONSOLE_HPP_INCLUDED
#define GG_CONSOLE_HPP_INCLUDED

#include "gg/core.hpp"

namespace gg
{
    class application;

    class console : public reference_counted
    {
    public:
        static console* get_invoker_console();

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
        virtual application* get_app() const = 0;
        virtual void set_controller(controller* ctrl) = 0;
        virtual controller* get_controller() const = 0;
        virtual void set_name(std::string name) = 0;
        virtual std::string get_name() const = 0;
        virtual void open() = 0;
        virtual void close() = 0;
        virtual bool is_opened() const = 0;
        virtual void on_close(std::function<void(console*)> callback) = 0;
        virtual output* create_output() = 0;
        virtual void clear() = 0;
    };
};

#endif // GG_CONSOLE_HPP_INCLUDED
