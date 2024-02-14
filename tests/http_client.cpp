// #include <functional>
// #include <cstdio>
// #include <cfenv>
#include <cstdio>
#include <cstdlib>
#include <iostream>
// #include <istream>
// #include <sstream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
// #include <string.h>
#include <filesystem>
#include <vector>

// namespace fs = std::filesystem; 
using namespace std;

#include <iostream>
#include <vector>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <fstream>


class SimpleReadline
{
    private:
        std::vector<std::string> history;
        struct termios orig_termios;

        void enableRawMode();

        void disableRawMode();

    public:
        SimpleReadline();

        ~SimpleReadline();

        std::string readLine();
        void loadHistoryFromFile(const std::string &filePath);
        void appendHistoryToFile(const std::string &line, const std::string &filePath);
};




std::vector<std::string> history;
struct termios orig_termios;

void SimpleReadline::enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void SimpleReadline::disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

SimpleReadline::SimpleReadline()
{
    enableRawMode();
}

SimpleReadline::~SimpleReadline()
{
    disableRawMode();
}

std::string SimpleReadline::readLine()
{

    string line;
    char c;

    string red = "\e[31m";
    string bold_green = "\e[1m\e[32m";
    string reset = "\e[0m";
    string bold_white = "\e[1m\e[37m";

    string dir = "/"; 

    size_t pos = dir.find(getenv("HOME"));
    if(pos != std::string::npos)
    {
        dir.replace(pos, string(getenv("HOME")).length(), "~");
    }

    string USER = getenv("USER");
    string shell_no_color("@()$ ");
    string shell(red + USER + reset + "@(" + bold_green + dir + reset + ")$ ");

    int length(shell_no_color.size() + USER.size() + dir.size() + 1);

    string prompt(shell);

    int cursorPos(0);
    int historyIndex(history.size());
    cout << prompt;
    cout.flush();

    while((read(STDIN_FILENO, &c, 1) == 1) && (c != '\n'))
    {
        // functions
        if(c == '\033')
        {
            read(STDIN_FILENO, &c, 1);
            read(STDIN_FILENO, &c, 1);
            
            // Up and Down Arrows (History Navigation)
            if(c == 'A' || c == 'B')
            {
                // Up arrow
                if(c == 'A' && historyIndex > 0)
                {
                    if(!line.empty())
                    {
                        history[historyIndex] = line;
                    }

                    historyIndex--;
                    std::cout << "\033[2K\r" << prompt << history[historyIndex];
                    line = history[historyIndex];
                    cursorPos = line.length();
                    std::cout.flush();
                    continue;
                }

                // Down arrow
                if(c == 'B' && historyIndex < history.size())
                {
                    history[historyIndex] = line;
                    historyIndex++;
                    std::cout << "\033[2K\r" << prompt << history[historyIndex];
                    line = history[historyIndex];
                    cursorPos = line.length();
                    std::cout.flush();
                    continue;
                }

                // Down arrow while line is empty will do nothing
                if(c == 'B' && line.empty())
                {
                    continue;
                }

                // Down arrow at last entry
                if(c == 'B' && historyIndex >= history.size())
                {
                    if(historyIndex == history.size())
                    {
                        history[historyIndex] = line;
                        historyIndex++;
                    }
                
                    if (historyIndex - 1 == history.size())
                    {
                        history[historyIndex - 1] = line;
                    }
                    
                    std::cout << "\033[2K\r" << prompt;
                    line.clear();
                    cursorPos = line.length();
                    std::cout.flush();
                    continue;
                }

                continue;
            }

            // Right and Left Arrows (Cursor Movement)
            if(c == 'C' || c == 'D')
            {
                // Right arrow
                if(c == 'C')
                {
                    if(cursorPos < line.size())
                    {
                        cursorPos++;
                        std::cout << "\033[" << (0) << "C";
                        std::cout.flush();
                        continue;
                    }

                    continue;
                }

                // Left arrow
                if(c == 'D')
                {
                    if(cursorPos > 0)
                    {
                        cursorPos--;
                        std::cout << "\033[" << (0) << "D";
                        std::cout.flush();
                        continue;
                    }

                    continue;
                }

                continue;
            }

            // Ctrl+
            if(c == '1')
            {
                read(STDIN_FILENO, &c, 1); // read ';'
                read(STDIN_FILENO, &c, 1); // read '5'
                read(STDIN_FILENO, &c, 1); // read actual command (D for left, C for right)
            
                // Ctrl + Left
                if(c == 'D')
                {
                    while(cursorPos > 0 && line[cursorPos - 1] == ' ') 
                    {
                        cursorPos--;
                    }

                    while(cursorPos > 0 && line[cursorPos - 1] != ' ') 
                    {    
                        cursorPos--;
                    }

                    std::cout << "\033[2K\r" << prompt << line;
                    std::cout << "\033[" << (cursorPos + length) << "G";
                    std::cout.flush();
                    continue;
                }

                // Ctrl + Right
                if(c == 'C')
                {
                    while(cursorPos < line.size() && line[cursorPos + 1] != ' ')
                    {
                        cursorPos++;
                    }

                    while(cursorPos < line.size() && line[cursorPos + 1] == ' ')
                    {
                        cursorPos++;
                    }

                    std::cout << "\033[2K\r" << prompt << line;
                    std::cout << "\033[" << (cursorPos + length) << "G";
                    std::cout.flush();
                    continue;
                }
            }

            continue;
        }

        // Backspace
        if(c == '\x7F')
        {
            if (cursorPos > 0)
            {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
                std::cout << "\033[2K\r" << prompt << line;
                std::cout << "\033[" << (cursorPos + length) << "G";
                std::cout.flush();
                continue;
            }

            continue;
        }

        // if CTRL+D is pressed return exit
        if(c == 4)
        {
            return "exit";
        }

        // do nothing when these are pressed
        if(
            c == 1 ||   // CTRL+A
            c == 2 ||   // CTRL+B
            c == 5 ||   // CTRL+E
            c == 6 ||   // CTRL+F
            c == 7 ||   // CTRL+G
            c == 8 ||   // CTRL+H
            c == 9 ||   // CTRL+I
            c == 11 ||  // CTRL+K
            c == 12 ||  // CTRL+L
            c == 14 ||  // CTRL+N
            c == 15 ||  // CTRL+O
            c == 16 ||  // CTRL+P
            c == 18 ||  // CTRL+R
            c == 20 ||  // CTRL+T
            c == 21 ||  // CTRL+U
            c == 22 ||  // CTRL+V
            c == 23 ||  // CTRL+W
            c == 24 ||  // CTRL+X
            c == 25 ||  // CTRL+Y
            c == 29     /*  CTRL+] */)
        {
            continue;
        }

        // append normal letters to line
        else
        {
            if(cursorPos >= 0)
            {
                line.insert(cursorPos, 1, c);
                cursorPos++;
                std::cout << "\033[2K\r" << prompt << line;
                std::cout << "\033[" << (cursorPos + length) << "G";
                std::cout.flush();
                continue;
            }
        }
    }

    cout << endl;
    if(!line.empty())
    {    
        history.push_back(line);
    }

    return line;
}

void SimpleReadline::loadHistoryFromFile(const string &filePath)
{
    ifstream historyFile(filePath);
    if(historyFile.is_open())
    {
        string line;
        while(getline(historyFile, line))
        {
            history.push_back(line);  // use the history vector of your SimpleReadline object
        }

        historyFile.close();
    }
}

void SimpleReadline::appendHistoryToFile(const string &line, const string &filePath)
{
    ofstream historyFile(filePath, ios::app); // Open file in append mode
    if (historyFile.is_open())
    {
        historyFile << line << endl;  // Append the line and a newline character to the file
        historyFile.close();
    }
}



int main()
{
    const char *__server_ip_address__("192.168.17.228");
    long __sock__(0);
    uint16_t __port__(8001);
    struct sockaddr_in __server_sock__;
    char buffer[10000] = {0};
    const char *__prompt(" --> : ");
    string __msg_str__("");
    SimpleReadline srl;

    while(true)
    {
        // init socket

            if((__sock__ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("socket: ");
                close(__sock__);
                exit(EXIT_FAILURE);
            }

            __server_sock__.sin_family = AF_INET;
            __server_sock__.sin_port = htons(__port__);

            if(inet_pton(AF_INET, __server_ip_address__, &__server_sock__.sin_addr) < 0)
            {
                perror("inet_pton: ");
                close(__sock__);
                exit(EXIT_FAILURE);
            }

            if(connect(__sock__, (struct sockaddr *)&__server_sock__, sizeof(__server_sock__)) < 0)
            {
                perror("connect: ");
                close(__sock__);
                exit(EXIT_FAILURE);
            }

        __msg_str__ = srl.readLine();
        __msg_str__ += '\0';

        if(send(__sock__, __msg_str__.c_str(), __msg_str__.length(), 0) < 0)
        {
            perror("send: ");
            close(__sock__);
            exit(EXIT_FAILURE);
        }

        // look if server responds

            char __buf__[10000];
            for(int i = 0; i < 10000; ++i)
            {
                __buf__[i] = '\0';
            }

            while(read(__sock__, &__buf__, 10000) > 0);
            if(__buf__[0] != '\0')
            {
                cout << (__buf__);
                for(int i = 0; i < 10000; ++i)
                {
                    __buf__[i] = '\0';
                }
            }
    }

    close(__sock__);
    return 0;
}