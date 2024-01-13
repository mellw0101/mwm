#ifndef LAUNCHER_HPP
#define LAUNCHER_HPP

#include <unistd.h>

class Launcher
{
    public:
        static void
        program(char * program)
        {
            if (fork() == 0) 
            {
                setsid();
                execvp(program, (char *[]) { program, NULL });
            }
        }
    ;
};

#endif // LAUNCHER_HPP