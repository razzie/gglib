#include "c_filesystem.hpp"

using namespace gg;


c_file::c_file(const char* name)
{

}

c_file::~c_file()
{
}

std::string c_file::get_name() const
{
    return {};
}

std::string c_file::get_extension() const
{
    return {};
}

std::string c_file::get_full_name() const
{
    return {};
}

std::string c_file::get_path() const
{
    return {};
}

std::fstream&& c_file::get_fstream() const
{
    return std::move(std::fstream());
}

size_t c_file::get_size() const
{
    return {};
}



c_directory::c_directory(const char* name)
{

}

c_directory::~c_directory()
{
}

std::string c_directory::get_name() const
{
    return {};
}

std::string c_directory::get_full_name() const
{
    return {};
}

std::string c_directory::get_path() const
{
    return {};
}

enumerator<file*> c_directory::get_files() const
{
    return {};
}

enumerator<file*> c_directory::c_directory::get_files_recursive() const
{
    return {};
}

enumerator<directory*> c_directory::get_directories() const
{
    return {};
}

enumerator<directory*> c_directory::get_directories_recursive() const
{
    return {};
}
