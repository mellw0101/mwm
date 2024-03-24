#ifndef PIDMANAGER_H
#define PIDMANAGER_H


#include <cstring>
#include <limits>
#include <unistd.h>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
// #include <future>
#include <iostream>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <vector>
#include <sstream>
#include <vector>

using namespace std;

class PidManager {
    public:
        vector<string> pidInfo(pid_t pid) 
        {
            std::vector<std::string> subprocesses = getSubprocesses(pid);
            pid_t parentPid = getParentPid(pid);

            for (auto &sub : subprocesses) 
            {
                pidStatus(std::stoi(sub));
            }

            return subprocesses;
        }

        vector<string> getPidByName(const string &procName) 
        {
            DIR* dir;
            struct dirent* ent;
            std::string path;
            std::string line;
            std::vector<std::string> subprocesses;
            pid_t pid;
            std::vector<std::string> prosseses;

            if ((dir = opendir("/proc")) != NULL) 
            {
                while ((ent = readdir(dir)) != NULL) 
                {
                    // Check if the directory is a PID
                    if (ent->d_type == DT_DIR) 
                    {
                        path = std::string("/proc/") + ent->d_name + "/comm";
                        std::ifstream comm(path.c_str());
                        if (comm.good()) 
                        {
                            std::getline(comm, line);
                            if (line == procName) 
                            {
                                pid = std::stoi(ent->d_name);
                                findSubprocesses(pid);
                                prosseses.push_back(std::to_string(pid));
                            }
                        }
                    }
                }
                closedir(dir);
            }
            return prosseses;
        }

        pid_t getParentPid(pid_t pid) 
        {
            string path = "/proc/" + to_string(pid) + "/stat";
            ifstream statFile(path.c_str());
            string value;
            pid_t ppid = -1;

            if (statFile.good())
            {
                // Skip PID and comm fields
                statFile >> value; // PID
                statFile.ignore(numeric_limits<streamsize>::max(), ')'); // Skip comm
                statFile >> value; // State
                statFile >> ppid; // PPID
            }

            return ppid;
        }

        string getCorrectProssesName(const string &__launchName)
        {
            DIR* dir;
            struct dirent* ent;
            string path;
            string line;

            vector<string> parts;
            for (int i(0), start(0); i < __launchName.length(); ++i)
            {
                if (__launchName[i] == '-')
                {
                    string s = __launchName.substr(start, i - start);
                    parts.push_back(s);
                    start = i + 1;
                }

                if (i == (__launchName.length() - 1))
                {
                    string s = __launchName.substr(start, i - start);
                    parts.push_back(s);
                }
            }

            for (int i = 0; i < parts.size(); ++i)
            {
                if ((dir = opendir("/proc")) != NULL)
                {
                    while ((ent = readdir(dir)) != NULL)
                    {
                        if (ent->d_type == DT_DIR) // Check if the directory is a PID
                        {
                            path = std::string("/proc/") + ent->d_name + "/comm";
                            std::ifstream comm(path.c_str());
                            if (comm.good())
                            {
                                getline(comm, line);
                                if (line == parts[i])
                                {
                                    return parts[i];
                                }
                            }
                        }
                    }
                    closedir(dir);
                }
            }

            return string();
        }
        
        vector<string> getSubprocesses(pid_t parent_pid)
        {
            vector<string> subprocesses;
            DIR* dir;
            struct dirent* entry;
            string tasks_path = "/proc/" + to_string(parent_pid) + "/task/";

            dir = opendir(tasks_path.c_str());
            if (!dir) 
            {
                perror("opendir");
                return subprocesses;
            }

            while ((entry = readdir(dir)) != nullptr) 
            {
                if (entry->d_type == DT_DIR) 
                {
                    pid_t pid = atoi(entry->d_name);
                    if (pid > 0 && pid != parent_pid) // Exclude the parent pid and '.' or '..' directories
                    {
                        subprocesses.push_back(to_string(pid));
                    }
                }
            }

            closedir(dir);
            return subprocesses;
        }
        
        vector<string> findSubprocesses(int pid) 
        {
            vector<string> subprocesses;
            DIR* procDir = opendir("/proc");
            if (procDir == nullptr)
            {
                perror("opendir");
                return subprocesses;
            }

            struct dirent* entry;
            while ((entry = readdir(procDir)) != nullptr)
            {
                int id = atoi(entry->d_name);
                if (id > 0)
                {
                    string statPath = std::string("/proc/") + entry->d_name + "/stat";
                    ifstream statFile(statPath);
                    string line;
                    if (getline(statFile, line))
                    {
                        istringstream iss(line);
                        vector<string> tokens;
                        string token;
                        while (getline(iss, token, ' '))
                        {
                            tokens.push_back(token);
                        }

                        if (tokens.size() > 3)
                        {
                            try
                            {
                                // Check if the token is a valid number
                                if (tokens[3].find_first_not_of("0123456789") == std::string::npos)
                                {
                                    int ppid = std::stoi(tokens[3]);
                                    if (ppid == pid)
                                    {
                                        subprocesses.push_back(std::to_string(id));
                                    }
                                }  
                            }
                            catch (const invalid_argument &e)
                            {
                                cerr << "Invalid argument: " << e.what() << '\n';
                            }
                            catch (const out_of_range &e)
                            {
                                cerr << "Out of range: " << e.what() << '\n';
                            }
                        }
                    }
                }
            }

            closedir(procDir);
            return subprocesses;
        }
        
        vector<int> searchPIDsForTTY(const string &tty) 
        {
            vector<int> matchingPIDs;
            DIR* procDir = opendir("/proc");
            struct dirent* entry;

            if (procDir == nullptr)
            {
                cerr << "Failed to open /proc directory" << endl;
                return matchingPIDs;
            }

            while ((entry = readdir(procDir)) != nullptr)
            {
                string pidDir = entry->d_name;
                if (pidDir.find_first_not_of("0123456789") == std::string::npos)
                {
                    string fdPath = "/proc/" + pidDir + "/fd/";
                    DIR* fdDir = opendir(fdPath.c_str());

                    if (fdDir != nullptr) {
                        struct dirent* fdEntry;
                        while ((fdEntry = readdir(fdDir)) != nullptr)
                        {
                            string fdLink = fdPath + fdEntry->d_name;
                            char linkPath[1024];
                            ssize_t len = readlink(fdLink.c_str(), linkPath, sizeof(linkPath)-1);
                            if (len != -1)
                            {
                                linkPath[len] = '\0';
                                string linkPathStr = string(linkPath);
                                if (linkPathStr.find(tty) != std::string::npos)
                                {
                                    matchingPIDs.push_back(std::stoi(pidDir));
                                    int pid = std::stoi(pidDir);
                                    // debug(F, __func__, R, ": ", tty, ": ", V, "PID:", N, pidDir, VV, getProcessNameByPid(pid), R);
                                    break;
                                }
                            }
                        }

                        closedir(fdDir);
                    }
                }
            }

            closedir(procDir);
            return matchingPIDs;
        }
        
        vector<pair<int, string>> getPIDsWithTTYs() 
        {
            vector<pair<int, string>> pidTTYs;
            DIR* procDir = opendir("/proc");
            struct dirent* entry;

            if (procDir == nullptr)
            {
                cerr << "Failed to open /proc directory" << endl;
                return pidTTYs;
            }

            while ((entry = readdir(procDir)) != nullptr)
            {
                string pidDir = entry->d_name;
                if (pidDir.find_first_not_of("0123456789") == string::npos)
                {
                    string fdPath = "/proc/" + pidDir + "/fd/";
                    DIR* fdDir = opendir(fdPath.c_str());
                    int pid = std::stoi(pidDir);

                    if (fdDir != nullptr)
                    {
                        struct dirent* fdEntry;
                        while ((fdEntry = readdir(fdDir)) != nullptr)
                        {
                            std::string fdLink = fdPath + fdEntry->d_name;
                            char linkPath[1024];
                            ssize_t len = readlink(fdLink.c_str(), linkPath, sizeof(linkPath)-1);
                            if (len != -1)
                            {
                                linkPath[len] = '\0';
                                if (strstr(linkPath, "/dev/pts/") != nullptr || strstr(linkPath, "/dev/tty") != nullptr)
                                {
                                    pidTTYs.push_back({pid, std::string(linkPath)});
                                    // debug(F, __func__, ": ", V, "PID:", N, pid, " ",VV, getProcessNameByPid(pid), V, " tty:", VV, std::string(linkPath), R);
                                    pidInfo(pid);
                                    vector<string> subprocesses = findSubprocesses(pid);
                                    for (const string &sub : subprocesses)
                                    {
                                        pidInfo(stoi(sub));
                                    }

                                    break; // Assuming one TTY per PID, break here
                                }
                            }
                        }

                        closedir(fdDir);
                    }
                }
            }

            closedir(procDir);
            return pidTTYs;
        }
        
        string pidCmdLine(const pid_t& pid) 
        {
            string line = "/proc/" + to_string(pid) + "/cmdline";
            ifstream file;
            file.open(line);
            string var;
            stringstream buffer;
            while (getline(file, var))
            {
                buffer << var << '\n';
            }

            string result;
            result = buffer.str();
            // debug("pidCmdLine: " + QString::fromStdString(result));
            string test = result;
            ifstream iss(test);
            string token;
            vector<string> parts;
            while (getline(iss, token, ' '))
            {
                parts.push_back(token);
            }
            
            if (parts.size() == 1)
            {
                file.close();
                return "mainPid";
            }

            file.close();
            return string();
        }
        
        string pidStatus(const pid_t& pid) 
        {
            string line = "/proc/" + to_string(pid) + "/status";
            std::ifstream file;
            file.open(line);
            std::string var;
            std::stringstream buffer;
            while (getline(file, var))
            {
                buffer << var << '\n';
            }

            string result;
            result = buffer.str();
            // debug("pidStatus: ", result);
            file.close();
            return string();
        }
        
        string pidFileDescriptors(pid_t pid) 
        {
            string result = string();
            std::string path = "/proc/" + std::to_string(pid) + "/fd/";

            DIR *dir = opendir(path.c_str());
            if (dir == nullptr)
            {
                perror("opendir failed");
                return string();
            }

            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                // Skipping "." and ".." entries
                if (entry->d_name[0] != '.')
                {
                    result += entry->d_name;
                }
            }

            closedir(dir);
            return result;
        }
        
        std::string getProcessNameByPid(pid_t pid) 
        {
            string path = "/proc/" + std::to_string(pid) + "/comm";
            ifstream commFile(path);
            string name;

            if (commFile.good())
            {
                getline(commFile, name);
                return name;
            }
            else
            {
                return "Process not found";
            }
        }
        
        string getRunningProsseses(const string &Prossesname)
        {
            string name = getCorrectProssesName(Prossesname);
            // searchPIDsForTTY("pts");
            // searchPIDsForTTY("tty");
            // getPIDsWithTTYs();
            // return getPidByName(stoi(name));
            return "";
        }
        
    private:
        std::vector<int> pidList;
};

#endif // PIDMANAGER_H
