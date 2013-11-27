#ifndef C_INIPARSER_HPP_INCLUDED
#define C_INIPARSER_HPP_INCLUDED

#include "tinythread.h"
#include "gg/iniparser.hpp"

namespace gg
{
    class c_ini_parser : public ini_parser
    {
    public:
        class c_group : public group
        {
            friend class c_ini_parser;

            mutable tthread::mutex m_mutex;
            std::string m_name;
            std::list<entry> m_entries;

            void save(std::ostream&);

        public:
            c_group(std::string);
            c_group(std::string, std::list<entry>&&);
            c_group(c_group&&);
            ~c_group();
            std::string get_name() const;
            void set_name(std::string);
            entry& operator[] (std::string);
            entry* get_entry(std::string);
            const entry* get_entry(std::string) const;
            std::list<entry>& get_entries();
            const std::list<entry>& get_entries() const;
        };

        c_ini_parser(std::string file);
        c_ini_parser(const std::istream&);
        ~c_ini_parser();
        c_group& operator[] (std::string);
        c_group* get_group(std::string);
        const c_group* get_group(std::string) const;
        c_group* create_group(std::string);
        void remove_group(std::string);
        void remove_group(group*);
        void save(std::string file);
        void save(std::ostream&);

    private:
        mutable tthread::mutex m_mutex;
        std::list<c_group> m_groups;
    };
};

#endif // C_INIPARSER_HPP_INCLUDED
