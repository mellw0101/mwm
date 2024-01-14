#ifndef FILE_HPP
#define FILE_HPP

#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <dirent.h>

#include "str.hpp"
#include "Log.hpp"

class File
{
    public: // subclasses
        class search
        {
            public: // construcers and operators
                search(const std::string& filename)
                : file(filename) {}

                operator bool() const
                {
                    return file.good();
                }
            ;

            private: // private variables
                std::ifstream file;
            ;
        };
    ;
    public: // construcers
        File() {}
    ;
    public: // methods
        std::string
        find_png_icon(std::vector<const char *> dirs, const char * app)
        {
            std::string name = app;
            name += ".png";

            for (const auto & dir : dirs)
            {
                std::vector<std::string> files = list_dir_to_vector(dir);
                for (const auto & file : files)
                {
                    if (file == name)
                    {
                        return dir + file;
                    }
                }
            }
            return "";
        }

        bool
        check_if_binary_exists(const char * name)
        {
            std::vector<const char *> dirs = split_$PATH_into_vector();
            return check_if_file_exists_in_DIRS(dirs, name);
        }
    ;
    private: // variables
        Logger log;
        string_tokenizer st;
    ;
    private: // functions
        bool
        check_if_file_exists_in_DIRS(std::vector<const char *> dirs, const char * app)
        {
            std::string name = app;

            for (const auto & dir : dirs)
            {
                std::vector<std::string> files = list_dir_to_vector(dir);
                for (const auto & file : files)
                {
                    if (file == name)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        std::vector<std::string>
        list_dir_to_vector(const char * directoryPath) 
        {
            std::vector<std::string> files;
            DIR* dirp = opendir(directoryPath);
            if (dirp) 
            {
                struct dirent* dp;
                while ((dp = readdir(dirp)) != nullptr) 
                {
                    files.push_back(dp->d_name);
                }
                closedir(dirp);
            }
            return files;
        }

        std::vector<const char *>
        split_$PATH_into_vector()
        {
            str $PATH = get_env_var("PATH");
            // if ($PATH.is_nullptr())
            // {
            //     log_error("PATH environment variable not set");
            //     return {};
            // }

            const char * _PATH = $PATH.c_str();

            return st.tokenize(_PATH, ":");
        }

        const char *
        get_env_var(const char * var)
        {
            const char * _var = getenv(var);
            if (_var == nullptr)
            {
                return nullptr;
            }
            return _var;
        }
    ;
};

#endif // FILE_HPP