#ifndef GG_INIPARSER_HPP_INCLUDED
#define GG_INIPARSER_HPP_INCLUDED

#include <iostream>
#include <list>
#include <string>
#include "gg/refcounted.hpp"
#include "gg/optional.hpp"

namespace gg
{
    class ini_parser : public reference_counted
    {
    public:
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
            virtual std::string get_name() const = 0;
            virtual void set_name(std::string) = 0;
            virtual entry& operator[] (std::string) = 0;
            virtual entry* get_entry(std::string) = 0;
            virtual const entry* get_entry(std::string) const = 0;
            virtual std::list<entry>& get_entries() = 0;
            virtual const std::list<entry>& get_entries() const = 0;
        };

        static ini_parser* create(std::string file);
        static ini_parser* create(const std::istream&);
        virtual ~ini_parser() {}
        virtual group& operator[] (std::string) = 0;
        virtual group* get_group(std::string) = 0;
        virtual const group* get_group(std::string) const = 0;
        virtual group* create_group(std::string) = 0;
        virtual void remove_group(std::string) = 0;
        virtual void remove_group(group*) = 0;
        virtual void save(std::string file) = 0;
        virtual void save(std::ostream&) = 0;
    };
};

#endif // GG_INIPARSER_HPP_INCLUDED
