#ifndef C_INIPARSER_HPP_INCLUDED
#define C_INIPARSER_HPP_INCLUDED

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
        std::list<section*> m_sections;

        void parse(std::istream&);

    public:
        class c_section : public section
        {
            mutable tthread::mutex m_mutex;
            std::string m_name;
            std::list<entry*> m_entries;

        public:
            c_section(std::string);
            c_section(std::string, std::list<entry*>&&);
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
            std::list<entry*>& get_entries();
            const std::list<entry*>& get_entries() const;
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
        std::list<section*>& get_sections();
        const std::list<section*>& get_sections() const;

        void save(std::string file);
        void save(std::ostream&);
    };
};

#endif // C_INIPARSER_HPP_INCLUDED
