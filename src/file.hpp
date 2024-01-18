#ifndef FILE_HPP
#define FILE_HPP

#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <dirent.h>

#include "str.hpp"
#include "Log.hpp"

class Directory_Searcher 
{
    public:
        Directory_Searcher() {}

        void search(const std::vector<const char *>& directories, const std::string& searchString) 
        {
            results.clear();
            searchDirectories = directories;

            for (const auto& dir : searchDirectories) 
            {
                DIR *d = opendir(dir);
                if (d == nullptr) 
                {
                    log_error("opendir() failed for directory: " + std::string(dir));
                    continue;
                }

                struct dirent *entry;
                while ((entry = readdir(d)) != nullptr) 
                {
                    std::string fileName = entry->d_name;
                    if (fileName.find(searchString) != std::string::npos) 
                    {
                        std::string dir_name = dir;
                        results.push_back(dir_name + "/" + fileName);
                    }
                }

                closedir(d);
            }
        }

        const std::vector<std::string>& getResults() const 
        {
            return results;
        }
    ;
    private:
        std::vector<const char *> searchDirectories;
        std::vector<std::string> results;
        Logger log;
    ;
};
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
        std::string find_png_icon(std::vector<const char *> dirs, const char * app)
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
        bool check_if_binary_exists(const char * name)
        {
            std::vector<const char *> dirs = split_$PATH_into_vector();
            return check_if_file_exists_in_DIRS(dirs, name);
        }
        std::vector<std::string> search_for_binary(const char * name)
        {
            ds.search(split_$PATH_into_vector(), name);
            return ds.getResults();
        }
    ;
    private: // variables
        Logger log;
        string_tokenizer st;
        Directory_Searcher ds;
    ;
    private: // functions
        bool check_if_file_exists_in_DIRS(std::vector<const char *> dirs, const char * app)
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
        std::vector<std::string> list_dir_to_vector(const char * directoryPath) 
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
        std::vector<const char *> split_$PATH_into_vector()
        {
            str $PATH(get_env_var("PATH"));
            return st.tokenize($PATH.c_str(), ":");
        }
        const char * get_env_var(const char * var)
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