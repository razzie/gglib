#ifndef C_INIPARSER_HPP_INCLUDED
#define C_INIPARSER_HPP_INCLUDED

#include <list>
#include "tinythread.h"
#include "gg/iniparser.hpp"

namespace gg
{
    class c_ini_parser : public ini_parser
    {
    public:
        class c_section;
        class c_entry;

    private:
        mutable tthread::mutex m_mutex;
        std::list<grab_ptr<section*, true>> m_sections;

        void parse(std::istream&);

    public:
        class c_section : public section
        {
            mutable tthread::mutex m_mutex;
            std::string m_name;
            std::list<grab_ptr<entry*, true>> m_entries;

        public:
            c_section(std::string);
            c_section(std::string, std::list<grab_ptr<entry*, true>>&&);
            c_section(c_section&&);
            ~c_section();
            std::string get_name() const;
            void set_name(std::string);
            entry& operator[] (std::string);
            entry* get_entry(std::string);
            const entry* get_entry(std::string) const;
            entry* add_entry(std::string, std::string);
            void remove_entry(std::string);
            void remove_entry(entry*);
            enumerator<entry*> get_entries();
            enumerator<entry*> get_entries() const;
        };

        class c_entry : public entry
        {
            mutable tthread::mutex m_mutex;
            section* m_section;
            std::string m_key;
            std::string m_value;

        public:
            c_entry(section*, std::string, std::string = {});
            ~c_entry();
            section* get_section();
            const section* get_section() const;
            std::string get_key() const;
            void set_key(std::string);
            std::string get_value() const;
            void set_value(std::string);
        };

        c_ini_parser(std::string file);
        c_ini_parser(std::istream&);
        ~c_ini_parser();

        entry* get_entry(std::string, std::string);
        const entry* get_entry(std::string, std::string) const;
        entry* add_entry(std::string section, std::string key, std::string value = {});
        void remove_entry(std::string, std::string);
        void remove_entry(entry*);

        section& operator[] (std::string);
        section* get_section(std::string);
        const section* get_section(std::string) const;
        section* create_section(std::string);
        void remove_section(std::string);
        void remove_section(section*);
        enumerator<section*> get_sections();
        enumerator<section*> get_sections() const;

        void save(std::string file) const;
        void save(std::ostream&) const;
    };
};

#endif // C_INIPARSER_HPP_INCLUDED
