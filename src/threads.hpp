#ifndef THREADS_HPP
#define THREADS_HPP

// #include <unordered_map>
// #include <vector>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>
#include "data.hpp"
// #include "structs.hpp"
#include "Log.hpp"
using namespace std;

typedef enum __Data__Type__t__ {
    Integer,
    FloatingPoint,
    String,
    // Add more types as needed
} DataType;

typedef struct {
    DataType type;
    void* data;
} TypedData;

// Creation
inline TypedData* createIntegerData(int value) {
    int* intData = new int(value);
    TypedData* typedData = new TypedData{Integer, intData};
    return typedData;

}

// Assuming similar functions for FloatingPoint and String

// Access
template <typename T>
T* getDataAs(TypedData* typedData, DataType expectedType) {
    if (typedData->type != expectedType) {
        loutE << "Data type mismatch!" << std::endl;
        return nullptr; // Type mismatch, return nullptr or handle error as appropriate

    }
    return static_cast<T*>(typedData->data);

}

// Deletion
inline void deleteTypedData(TypedData* typedData) {
    if (!typedData) return;

    // Add specific deletions based on `DataType`
    switch (typedData->type) {
        case DataType::Integer:
            delete static_cast<int*>(typedData->data);
            break;
        // Handle other types similarly
        default:
            break;
    }

    delete typedData;
}

typedef struct {
    char **str = new char*;
    uint64_t **data = new uint64_t*;

} __inter__data__struct__t__;

typedef struct {
    __inter__data__struct__t__ *intern_data;
    void *v = new void*;

} data_t;

typedef struct {
    double *time      = new double;
    char  **func_name = new char*;

} time_data_t;

inline int alloc_() {
    // Allocate and initialize internal data
    __inter__data__struct__t__* internalData = new __inter__data__struct__t__[2];

    // Example initialization of the first set of internal data
    internalData[0].str = new char*;
    *(internalData[0].str) = new char[6]; // Example for "hello"
    std::strcpy(*(internalData[0].str), "hello");

    internalData[0].data = new uint64_t*;
    *(internalData[0].data) = new uint64_t(42); // Example data

    // data_t usage
    data_t myData;
    myData.intern_data = internalData; // Assign the allocated internal data

    // Example usage of void*
    int* exampleInt = new int(7); // Dynamically allocate an int
    myData.v = static_cast<void*>(exampleInt); // Store its address in v

    // Usage demonstration
    std::cout << "String: " << *(*(myData.intern_data[0].str)) << std::endl;
    std::cout << "Number: " << *(*(myData.intern_data[0].data)) << std::endl;
    std::cout << "Void*: " << *(static_cast<int*>(myData.v)) << std::endl;

    // Clean up
    delete[] *(internalData[0].str);
    delete internalData[0].str;

    delete *(internalData[0].data);
    delete internalData[0].data;

    delete[] internalData; // This also cleans up myData.intern_data

    delete static_cast<int*>(myData.v);

    return 0;
}

// #define FUNC_TIMER_SIGNAL 145

typedef struct __function__timer__data__struct__t__ {
    string functionName;
    chrono::microseconds executionTime;
    Signal<chrono::microseconds> endSig[2];

    __function__timer__data__struct__t__(const string& name, const chrono::microseconds& time)
    : functionName(name), executionTime(time) {
        endSig[0] = [&](const chrono::microseconds &time) -> void {

            
        };

    }

} FuncTimerStruct;

// class ScopeTimer {
//     private:
//         string scopeName;
//         chrono::high_resolution_clock::time_point startTime;
//         chrono::microseconds &executionTime;

//     public:
//         ScopeTimer(const string& name, chrono::microseconds &executionTimeRef)
//             : scopeName(name), executionTime(executionTimeRef) {
//             startTime = chrono::high_resolution_clock::now();
//         }

//         ~ScopeTimer() {
//             auto endTime = chrono::high_resolution_clock::now();
//             executionTime = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
//             loutI << scopeName << " executed in " << executionTime.count() << " microseconds." << loutEND;
//         }
// };

/* void processData(void* rawData) {
    auto* data = static_cast<data_t*>(rawData);

    // Example: just print the first element of `w` and `s` to verify
    std::cout << "w[0]: " << data->w[0] << ", s[0]: " << static_cast<int>(data->s[0]) << std::endl;

    // Remember to properly manage the memory for data_t
    delete[] data->w;
    delete[] data->s;
    delete static_cast<void**>(data->v); // Assuming 'v' was intended to point to a 'void*' array
    delete data; // Assuming you want to manage data_t's lifetime here
} */


class Thread; class Thread {
    private:
        thread t;
        size_t layer = 0;

    public:
        template <typename Callable>
        Thread(Callable&& func) {
            t = std::thread(std::forward<Callable>(func));

        }
        template<typename...Args>
        Thread&operator()(Args&&...args){
            

        }
    
};

class root_thread_t; class root_thread_t {
        Signal<uint64_t> t_sig;
        vector<thread*> t;
        void **__data;
        
    public:
        root_thread_t() : __data(new void *) {}

        int run() {    
            while (true) {

                
            }
            return 0;
            
        }
    
};

#endif // THREADS_HPP