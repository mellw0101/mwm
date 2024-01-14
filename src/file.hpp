#ifndef FILE_HPP
#define FILE_HPP

#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <dirent.h>
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
    
    public: // construcers and operators
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
        check_if_binary_exists(const char * file_name)
        {
            split_$PATH_into_vector();
            return false;
        }
    ;

    private: // variables
        Logger log;

    private: // functions
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
            const char * $PATH = get_env_var("PATH");
            if ($PATH == nullptr)
            {
                return {};
            }
            log_info(std::string($PATH));
            return {};
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