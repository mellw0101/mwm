#include <cstring>
#include <vector>

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
            tokens.clear();

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
                is_null = false;
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
                is_null = (data == nullptr);
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

        // Get the length of the string
        size_t size() const 
        {
            return length;
        }

        // Method to check if the string is empty
        bool isEmpty() const 
        {
            return length == 0;
        }

        bool is_nullptr() const
        {
            return is_null;
        }
    ;
    private: // variables
        char* data;
        size_t length;
        bool is_null = false;
    ;
};

class fast_str_vector
{
    public: // operators
        operator std::vector<str>() const
        {
            return data;
        }
    ;
    public: // [] operator Access an element in the vector
        str operator[](size_t index) const 
        {
            return data[index];
        }
    ;
    public: // methods
        void // Add a string to the vector
        push_back(str str)
        {
            data.push_back(str);
        }

        void // Add a string to the vector
        append(str str)
        {
            data.push_back(str);
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
        std::vector<str> data; // Internal vector to store const char* strings
    ;
};