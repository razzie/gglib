#ifndef C_LOGGER_HPP_INCLUDED
#define C_LOGGER_HPP_INCLUDED

#include <map>
#include "tinythread.h"
#include "fast_mutex.h"
#include "threadglobal.hpp"
#include "gg/logger.hpp"

namespace gg
{
    class c_logger : public std::streambuf, public logger
    {
        mutable tthread::fast_mutex m_mutex;
        std::map<tthread::thread::id, std::string> m_sync_log;
        recursive_thread_global<std::ostream*> m_hooks;
        std::streambuf* m_cout_rdbuf;
        std::ostream* m_stream;
        std::fstream* m_file;
        bool m_log_to_file;
        bool m_timestamp;

        friend class logger::scoped_hook;

        class wrapper : public std::streambuf
        {
            c_logger* m_logger;
        protected:
            int overflow(int c = std::char_traits<char>::eof()) { return m_logger->overflow(c); }
            int sync() { return m_logger->sync(); }
        public:
            wrapper(c_logger* l) : m_logger(l) {}
            ~wrapper() {}
        };

    protected:
        c_logger();
        ~c_logger();
        void push_hook(std::ostream&);
        void pop_hook();
        // inherited from std::streambuf
        int overflow(int c = std::char_traits<char>::eof());
        int sync();

    public:
        static c_logger* get_instance();
        void enable_cout_hook();
        void disable_cout_hook();
        void enable_timestamp();
        void disable_timestamp();
        void log_to_stream(std::ostream& = std::cout);
        void log_to_file(std::string);
    };
};

#endif // C_LOGGER_HPP_INCLUDED
