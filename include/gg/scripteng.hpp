#ifndef GG_SCRIPTENG_HPP_INCLUDED
#define GG_SCRIPTENG_HPP_INCLUDED

#include <iostream>
#include <vector>
#include "gg/types.hpp"
#include "gg/var.hpp"

namespace gg
{
    class script_engine
    {
    protected:
        virtual ~script_engine() {}

    public:
        struct arg
        {
            enum class type
            {
                NONE, ANY, INT, FLOAT, STRING, VARARG
            };

            type m_type;
            var m_value;

            int get_int() { return m_value.get<int>(); }
            float get_float() { return m_value.get<float>(); }
            std::string get_string() { return m_value.get<std::string>(); }
        };

        typedef bool(*callback)(std::vector<arg>);

        virtual void add_command(std::string cmd,
                                 std::vector<arg::type> args,
                                 callback cb) = 0;
        virtual void remove_command(std::string cmd) = 0;
        virtual bool exec(std::string cmd,
                          std::vector<arg> args,
                          std::ostream& output = std::cout) const = 0;
        virtual bool parse_and_exec(std::string cmd_line,
                                    std::ostream& output = std::cout) const = 0;
    };
};

#endif // GG_SCRIPTENG_HPP_INCLUDED
