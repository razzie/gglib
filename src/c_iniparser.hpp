#ifndef C_INIPARSER_HPP_INCLUDED
#define C_INIPARSER_HPP_INCLUDED

#include "tinythread.h"
#include "gg/iniparser.hpp"

namespace gg
{
    class c_ini_parser
    {
        mutable application* m_app;

    public:
        class c_parse_result : public parse_result
        {
        public:
            class c_group : public group
            {
                tthread::mutex m_mutex;
                std::list<entry> m_entries;

            public:
                c_group();
                ~c_group();
                entry* operator[] (std::string);
                const entry* operator[] (std::string) const;
                entry* get_entry(std::string);
                const entry* get_entry(std::string) const;
                std::list<entry>& get_entries();
                const std::list<entry>& get_entries() const;
            };

            group* get_group(std::string);
            const group* get_group(std::string) const;
            group* create_group(std::string);
            void save();
        };

        enum mode
        {
            READ,
            WRITE,
            READ_WRITE
        };

        ini_parser(application* app);
        ~ini_parser();
        application* get_app() const;
        parse_result* open(std::string, mode = READ_WRITE) const;
        parse_result* open(std::iostream&, mode = READ_WRITE) const;
        parse_result* open(std::istream&) const;
        parse_result* open(std::ostream&) const;
    };
};

#endif // C_INIPARSER_HPP_INCLUDED
