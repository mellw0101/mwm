#include <thread>
using namespace std;
#include <atomic>
#include <thread>
#include <functional>


class Thread : thread {
public:
    std::atomic<bool> active{true}; // Atomic flag to control thread activity
    std::atomic<bool> paused{false}; // Atomic flag to pause/resume the thread
    std::thread worker; // The managed thread

    // Constructor
    Thread(std::function<void()> func) : worker([this, func]() {
        while (active) {
            if (!paused) {
                func(); // Execute the passed function if not paused
                
            }

        }

    }) {}
    // Destructor to ensure proper thread joining
    ~Thread() {
        if (worker.joinable()) {
            worker.join();

        }

    }
    // Pause the thread
    void pause() {
        paused = true;

    }
    // Resume the thread
    void resume() {
        paused = false;

    }
    // Stop the thread
    void stop() {
        active = false;

    }

    // Example method to send signals or manipulate the thread
    void sendSignal(/* parameters for the signal */) {
        // Implementation depends on what kind of signals you want to send
        // This could adjust atomic flags or set conditions for the thread's operation
    }

    // Additional functionality to create child threads or merge could be implemented here
};