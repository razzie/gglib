#ifndef GG_SCRIPTENG_HPP_INCLUDED
#define GG_SCRIPTENG_HPP_INCLUDED

#include <iostream>
#include "gg/types.hpp"
#include "gg/var.hpp"

namespace gg
{
    class script_engine
    {
    protected:
        virtual ~script_engine() {}

    public:
        enum arg_type
        {
            NONE, ANY, INT, FLOAT, STRING, VARARG
        };

        struct arg
        {
            arg_type m_type;
            var m_value;
        };

        typedef bool(*callback)(std::vector<arg>);

        virtual void add_command(std::string cmd,
                                 std::initializer_list<arg_type> args,
                                 callback cb) = 0;
        virtual void remove_command(std::string cmd) = 0;
        virtual bool exec(std::string cmd,
                          std::initializer_list<arg> args,
                          std::ostream& output = std::cout) const = 0;
        virtual bool parse_and_exec(std::string cmd_line,
                                    std::ostream& output = std::cout) const = 0;
    };
};

#endif // GG_SCRIPTENG_HPP_INCLUDED
