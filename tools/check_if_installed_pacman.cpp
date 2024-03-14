#include <string>
#include <filesystem>
#include <iostream>

int check_package_installed(const std::string& packageName)
{
    std::string packagePath = "/var/lib/pacman/local/";

    // Iterate through directories in /var/lib/pacman/local
    for (const auto& entry : std::__fs::filesystem::directory_iterator(packagePath))
    {
        if (entry.is_directory() && entry.path().filename().string().find(packageName) != std::string::npos)
        {
            std::cout << packageName << " is already installed." << std::endl;
            return 0; // Package is installed
        }
    }

    std::cout << packageName << " is not installed." << std::endl;
    return 1; // Package is not installed
}

int main(int argc, char *argv[])
{
    if (argc != 1) return 1;
    return check_package_installed(argv[0]);
}