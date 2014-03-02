#include <dirent.h>
#include <windows.h>
#include "c_filesystem.hpp"

using namespace gg;


static std::string get_basename(const std::string& name)
{
    size_t len = name.size();

    for (size_t i = len; i >= 0; --i)
    {
        if (name[i] == '/') return &(name.c_str())[i];
    }

    return name;
}

static std::string get_extension(const std::string& name)
{
    size_t len = name.size();

    for (size_t i = len; i >= 0; --i)
    {
        if (name[i] == '.') return &(name.c_str())[i];
        else if (name[i] == '/') return {};
    }

    return {};
}

static std::string get_path(const std::string& name)
{
    size_t len = name.size();

    for (size_t i = len; i >= 0; --i)
    {
        if (name[i] == '/') return name.substr(0, i);
    }

    return name;
}


c_file::c_file(std::string name)
 : m_name(name)
{
}

c_file::~c_file()
{
}

std::string c_file::get_name() const
{
    return ::get_basename(m_name);
}

std::string c_file::get_extension() const
{
    return ::get_extension(m_name);
}

std::string c_file::get_full_name() const
{
    return m_name;
}

std::string c_file::get_path() const
{
    return ::get_path(m_name);
}

std::fstream&& c_file::get_fstream() const
{
    return std::move(std::fstream(m_name));
}

size_t c_file::get_size() const
{
    std::ifstream file(m_name, std::ios::binary | std::ios::ate);
    return file.tellg();
}


directory* directory::open(std::string name)
{
    return new c_directory(name);
}

c_directory::c_directory(std::string name)
 : m_name(name)
{
    char dir_end = m_name[m_name.size()-1];

    if (dir_end != '/' && dir_end != '\\')
        m_name += "/";
}

c_directory::~c_directory()
{
}

std::string c_directory::get_name() const
{
    return ::get_basename(m_name);
}

std::string c_directory::get_full_name() const
{
    return m_name;
}

std::string c_directory::get_path() const
{
    return ::get_path(m_name);
}

void c_directory::get_files(std::list<grab_ptr<file*, true>>& files) const
{
    DIR *dir;
    struct dirent *ent;
    std::string fname;

    if ((dir = opendir( m_name.c_str() )) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            fname = m_name + ent->d_name;

            if (GetFileAttributes(fname.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
                continue;

            file* f = new c_file(fname);
            files.push_back(f);
            f->drop();
        }
        closedir(dir);
    }
}

enumerator<file*> c_directory::get_files() const
{
    std::list<grab_ptr<file*, true>> files;
    get_files(files);
    return make_enumerator<file*>(files);
}

enumerator<file*> c_directory::c_directory::get_files_recursive() const
{
    std::list<grab_ptr<file*, true>> files;
    std::list<grab_ptr<directory*, true>> dirs;

    get_directories(dirs, true);
    for (directory* d : dirs)
    {
        static_cast<c_directory*>(d)->get_files(files);
    }

    return make_enumerator<file*>(files);
}

void c_directory::get_directories(std::list<grab_ptr<directory*, true>>& dirs, bool recursive) const
{
    DIR *dir;
    struct dirent *ent;
    std::string dname;

    if ((dir = opendir( m_name.c_str() )) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            dname = m_name + ent->d_name;

            if (GetFileAttributes(dname.c_str()) != FILE_ATTRIBUTE_DIRECTORY)
                continue;

            if (ent->d_name[0] == '.')
                continue;

            c_directory* d = new c_directory(dname);
            dirs.push_back(d);
            if (recursive) d->get_directories(dirs, true);
            d->drop();
        }
        closedir(dir);
    }
}

enumerator<directory*> c_directory::get_directories() const
{
    std::list<grab_ptr<directory*, true>> dirs;
    get_directories(dirs);
    return make_enumerator<directory*>(dirs);
}

enumerator<directory*> c_directory::get_directories_recursive() const
{
    std::list<grab_ptr<directory*, true>> dirs;
    get_directories(dirs, true);
    return make_enumerator<directory*>(dirs);
}
