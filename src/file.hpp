#ifndef FILE_HPP
#define FILE_HPP

#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <dirent.h>

#include "Log.hpp"

class fast_vector 
{
    public: // operators
        operator std::vector<const char*>() const
        {
            return data;
        }
    ;
    public: // Destructor
        ~fast_vector() 
        {
            for (auto str : data) 
            {
                delete[] str;
            }
        }
    ;
    public: // [] operator Access an element in the vector
        const char* operator[](size_t index) const 
        {
            return data[index];
        }
    ;
    public: // methods
        void // Add a string to the vector
        push_back(const char* str)
        {
            char* copy = new char[strlen(str) + 1];
            strcpy(copy, str);
            data.push_back(copy);
        }

        void // Add a string to the vector
        append(const char* str)
        {
            char* copy = new char[strlen(str) + 1];
            strcpy(copy, str);
            data.push_back(copy);
        }

        size_t // Get the size of the vector
        size() const 
        {
            return data.size();
        }

        size_t // get the index of the last element in the vector
        index_size() const
        {
            if (data.size() == 0)
            {
                return 0;
            }

            return data.size() - 1;
        }

        void // Clear the vector
        clear()
        {
            data.clear();
        }
    ;
    private: // variabels
        std::vector<const char*> data; // Internal vector to store const char* strings
    ;
};

class string_tokenizer 
{
    public: // constructors and destructor
        string_tokenizer() {}

        string_tokenizer(const char* input, const char* delimiter) 
        {
            // Copy the input string
            str = new char[strlen(input) + 1];
            strcpy(str, input);

            // Tokenize the string using strtok() and push tokens to the vector
            char* token = strtok(str, delimiter);
            while (token != nullptr) 
            {
                tokens.push_back(token);
                token = strtok(nullptr, delimiter);
            }
        }

        ~string_tokenizer() 
        {
            delete[] str;
        }
    ;
    public: // methods
        const fast_vector &
        tokenize(const char* input, const char* delimiter)
        {
            // Copy the input string
            str = new char[strlen(input) + 1];
            strcpy(str, input);

            // Tokenize the string using strtok() and push tokens to the vector
            char* token = strtok(str, delimiter);
            while (token != nullptr) 
            {
                tokens.append(token);
                token = strtok(nullptr, delimiter);
            }
            return tokens;
        }

        const fast_vector & 
        get_tokens() const 
        {
            return tokens;
        }

        void
        clear()
        {
            tokens.clear();
        }
    ;
    private: // variables
        char* str;
        fast_vector tokens;
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
            fast_vector dirs = split_$PATH_into_vector();
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

        fast_vector
        split_$PATH_into_vector()
        {
            const char * $PATH = get_env_var("PATH");
            if ($PATH == nullptr)
            {
                log_error("PATH environment variable not set");
                return {};
            }
            
            st.clear();
            fast_vector dirs = st.tokenize($PATH, ":");
            return dirs;
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