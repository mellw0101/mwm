#include <cstddef>

class str 
{
    public:
        // Constructor
        str(const char* str = "") 
        {
            length = manualStrlen(str);
            data = new char[length + 1];
            manualStrcpy(data, str);
        }

        // Destructor
        ~str() 
        {
            delete[] data;
        }

        // Copy Constructor
        str(const str& other) 
        {
            length = other.length;
            data = new char[length + 1];
            manualStrcpy(data, other.data);
        }

        // Copy Assignment Operator
        str& operator=(const str& other) 
        {
            if (this != &other) 
            {
                delete[] data;
                length = other.length;
                data = new char[length + 1];
                manualStrcpy(data, other.data);
            }
            return *this;
        }

        // Accessor for C-style string
        const char* c_str() const 
        {
            return data;
        }

        // Manual implementation of append
        void 
        append(const char* str) 
        {
            size_t newLength = length + manualStrlen(str);
            char* newData = new char[newLength + 1];

            manualStrcpy(newData, data);
            manualStrcpy(newData + length, str);

            delete[] data;
            data = newData;
            length = newLength;
        }

        // More methods like insert, replace, etc., can be implemented manually
    ;

    private:
        char* data;
        size_t length; // To keep track of the string length

        // Manual implementation to find the length of a C-style string
        static 
        size_t manualStrlen(const char* str) 
        {
            size_t len = 0;
            while (str && str[len] != '\0') 
            {
                ++len;
            }
            return len;
        }

        // Manual implementation to copy a C-style string
        static void 
        manualStrcpy(char* dest, const char* src) 
        {
            while (*src) 
            {
                *dest++ = *src++;
            }
            *dest = '\0';
        }
    ;
};