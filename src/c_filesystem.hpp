#ifndef C_FILESYSTEM_HPP_INCLUDED
#define C_FILESYSTEM_HPP_INCLUDED

#include <dirent.h>
#include "gg/filesystem.hpp"

namespace gg
{
    class c_file : public file
    {

    public:
        c_file(const char*);
        ~c_file();
        std::string get_name() const;
        std::string get_extension() const;
        std::string get_full_name() const;
        std::string get_path() const;
        std::fstream&& get_fstream() const;
        size_t get_size() const;
    };

    class c_directory : public directory
    {

    public:
        c_directory(const char*);
        ~c_directory();
        std::string get_name() const;
        std::string get_full_name() const;
        std::string get_path() const;
        enumerator<file*> get_files() const;
        enumerator<file*> get_files_recursive() const;
        enumerator<directory*> get_directories() const;
        enumerator<directory*> get_directories_recursive() const;
    };
};

#endif // C_FILESYSTEM_HPP_INCLUDED
