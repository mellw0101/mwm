#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <unistd.h>
#include "file.hpp"

class Launcher
{
    public:
        void program(char * program)
        {
            if (!file.check_if_binary_exists(program))
            {
                return;
            }
            if (fork() == 0) 
            {
                setsid();
                execvp(program, (char *[]) { program, NULL });
            }
        }
    ;
    private:
        File file;
    ;
};

#endif // LAUNCHER_HPP