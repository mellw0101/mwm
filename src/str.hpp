#include <cstring>

class str 
{
    public: // Constructor
        str(const char* str = "") 
        {
            length = strlen(str);
            data = new char[length + 1];
            strcpy(data, str);
        }
    ;
    public: // Copy constructor
        str(const str& other) 
        {
            length = other.length;
            data = new char[length + 1];
            strcpy(data, other.data);
        }
    ;
    public: // Move constructor
        str(str&& other) noexcept 
        : data(other.data), length(other.length) 
        {
            other.data = nullptr;
            other.length = 0;
        }
    ;
    public: // Destructor
        ~str() 
        {
            delete[] data;
        }
    ;
    public: // Copy assignment operator
        str& operator=(const str& other) 
        {
            if (this != &other) 
            {
                delete[] data;
                length = other.length;
                data = new char[length + 1];
                strcpy(data, other.data);
            }
            return *this;
        }
    ;
    public: // Move assignment operator
        str& operator=(str&& other) noexcept 
        {
            if (this != &other) 
            {
                delete[] data;
                data = other.data;
                length = other.length;
                other.data = nullptr;
                other.length = 0;
            }
            return *this;
        }
    ;
    public: // Concatenation operator
        str operator+(const str& other) const 
        {
            str result;
            result.length = length + other.length;
            result.data = new char[result.length + 1];
            strcpy(result.data, data);
            strcat(result.data, other.data);
            return result;
        }
    ;
    public: // methods
        // Access to underlying C-string
        const char * c_str() const 
        {
            return data;
        }
    ;
    private: // variables
        char* data;
        size_t length;
    ;
};