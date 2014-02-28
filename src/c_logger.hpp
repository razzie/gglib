#ifndef C_LOGGER_HPP_INCLUDED
#define C_LOGGER_HPP_INCLUDED

#include <map>
#include "tinythread.h"
#include "fast_mutex.h"
#include "threadglobal.hpp"
#include "gg/logger.hpp"
#include "c_timer.hpp"

namespace gg
{
    class nullstream : public std::streambuf, public std::ostream
    {
    public:
        nullstream();
        ~nullstream();
        int overflow(int c = std::char_traits<char>::eof());
        int sync();
    };

    class c_logger : public std::streambuf, public logger
    {
        mutable tthread::fast_mutex m_mutex;
        std::map<tthread::thread::id, std::string> m_sync_log;
        recursive_thread_global<std::ostream*> m_hooks;
        c_timer m_timer;
        std::ostream* m_cout;
        std::streambuf* m_cout_rdbuf;
        std::ostream* m_stream;
        std::fstream* m_file;
        console* m_console;
        bool m_log_to_file;
        bool m_timestamp;
        mutable nullstream m_nullstream;

        friend class logger::scoped_hook;

        class wrapper : public std::streambuf
        {
            c_logger* m_logger;
        protected:
            int overflow(int c = std::char_traits<char>::eof());
            int sync();
        public:
            wrapper(c_logger* l);
            ~wrapper();
        };

        c_logger();
        ~c_logger();
        void add_timestamp(std::string&) const;

    protected:
        void push_hook(std::ostream&);
        void pop_hook();
        void flush_thread(tthread::thread::id);
        // inherited from std::streambuf
        int overflow(int c = std::char_traits<char>::eof());
        int sync();

    public:
        static c_logger* get_instance();
        void register_cout(std::ostream&);
        void enable_cout_hook();
        void disable_cout_hook();
        void enable_timestamp();
        void disable_timestamp();
        void log_to_stream(std::ostream& = std::cout);
        void log_to_file(std::string);
        void log_to_console(console*);
        std::ostream& get_nullstream() const;
    };
};

#endif // C_LOGGER_HPP_INCLUDED
