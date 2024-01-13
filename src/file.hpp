#ifndef FILE_HPP
#define FILE_HPP

#include <string>
#include <fstream>

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
};

#endif // FILE_HPP