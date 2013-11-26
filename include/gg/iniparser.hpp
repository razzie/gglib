#ifndef GG_INIPARSER_HPP_INCLUDED
#define GG_INIPARSER_HPP_INCLUDED

#include <list>
#include <string>
#include "gg/refcounted.hpp"
#include "gg/optional.hpp"

namespace gg
{
    class application;

    class ini_parser
    {
    protected:
        virtual ~ini_parser() {}

    public:
        class parse_result : public reference_counted
        {
        public:
            /*class entry
            {
            protected:
                virtual ~entry() {}
            public:
                virtual std::string get_key() const = 0;
                virtual std::string get_value() const = 0;
            };*/
            struct entry
            {
                std::string key;
                std::string value;
            };

            class group
            {
            protected:
                virtual ~group() {}
            public:
                virtual entry* operator[] (std::string) = 0;
                virtual const entry* operator[] (std::string) const = 0;
                virtual entry* get_entry(std::string) = 0;
                virtual const entry* get_entry(std::string) const = 0;
                virtual std::list<entry>& get_entries() = 0;
                virtual const std::list<entry>& get_entries() const = 0;
            };

            virtual group* get_group(std::string) = 0;
            virtual const group* get_group(std::string) const = 0;
            virtual group* create_group(std::string) = 0;
            virtual void save() = 0;
        };

        enum mode
        {
            READ,
            WRITE,
            READ_WRITE
        };

        virtual application* get_app() const = 0;
        virtual parse_result* open(std::string, mode = READ_WRITE) const = 0;
        virtual parse_result* open(std::iostream&, mode = READ_WRITE) const = 0;
        virtual parse_result* open(std::istream&) const = 0;
        virtual parse_result* open(std::ostream&) const = 0;
    };
};

#endif // GG_INIPARSER_HPP_INCLUDED
