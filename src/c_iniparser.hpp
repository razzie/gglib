#ifndef C_INIPARSER_HPP_INCLUDED
#define C_INIPARSER_HPP_INCLUDED

#include "tinythread.h"
#include "gg/iniparser.hpp"

namespace gg
{
    class c_ini_parser : public ini_parser
    {
        mutable application* m_app;

    public:
        class c_parse_result : public ini_parser::parse_result
        {
            mutable tthread::mutex m_mutex;

        public:
            class c_group : public group
            {
                mutable tthread::mutex m_mutex;
                std::list<entry> m_entries;

                void save(std::ostream&);

            public:
                c_group();
                c_group(std::list<entry>&&);
                ~c_group();
                entry* operator[] (std::string);
                const entry* operator[] (std::string) const;
                entry* get_entry(std::string);
                const entry* get_entry(std::string) const;
                std::list<entry>& get_entries();
                const std::list<entry>& get_entries() const;
            };

            c_parse_result(std::string file);
            c_parse_result(std::istream&);
            ~c_parse_result();
            c_group* get_group(std::string);
            const c_group* get_group(std::string) const;
            c_group* create_group(std::string);
            void remove_group(std::string);
            void remove_group(group*);
            void save(std::string file);
            void save(std::ostream&);
        };

        c_ini_parser(application* app);
        ~c_ini_parser();
        application* get_app() const;
        c_parse_result* open(std::string file) const;
        c_parse_result* open(std::istream&) const;
    };
};

#endif // C_INIPARSER_HPP_INCLUDED
