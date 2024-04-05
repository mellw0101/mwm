#include <thread>
using namespace std;
#include <atomic>
#include <thread>
#include <functional>


#include <atomic>
#include <thread>
#include <functional>

#include <atomic>
#include <thread>
#include <functional>
#include <exception>
#include <optional>

class Thread {
    public:
        template<typename Callable, typename... Args>
        Thread(Callable&& func, Args&&... args)
        : active(true), paused(false), worker([this, func = std::forward<Callable>(func), ...args = std::forward<Args>(args)]() {
            try {
                while (active) {
                    if (!paused) {
                        func(std::forward<Args>(args)...);
                        
                    }
                    std::this_thread::yield(); // Yield to avoid busy waiting
                    
                }

            } catch (...) {
                lastException = std::current_exception(); // Capture any exception
                // Optionally, notify about the exception here

            }
        }) {}

        ~Thread() {
            stop(); // Ensure the thread is signaled to stop

        }

        // Modified operations for thread safety and error handling
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
        Thread(Thread&&) = delete;
        Thread& operator=(Thread&&) = delete;

        void pause() { paused = true; }
        void resume() { paused = false; }
        void stop() {
            if (active) {
                active = false;
                if (worker.joinable()) {
                    worker.join();

                }

            }

        }

        bool isPaused() const { return paused; }
        bool isActive() const { return active; }
        thread::id getID() const { return worker.get_id(); }

        // New Functionality: Error handling
        bool hasExceptionOccurred() const { return static_cast<bool>(lastException); }
        void rethrowException() const {
            if (lastException) {
                std::rethrow_exception(lastException);

            }

        }

    private:
        atomic<bool> active;
        atomic<bool> paused;
        thread worker;
        exception_ptr lastException; // Holds the last exception thrown by the thread

};

class TimedDataSender {
    public:
        // Constructor: Takes the interval in milliseconds and the task to perform
        TimedDataSender(unsigned interval, std::function<void()> task)
        : interval_(interval), task_(std::move(task)), active_(true) {
            worker_ = std::thread([this]() { this->loop(); });
        }

        ~TimedDataSender() {
            // Signal the loop to stop and join the thread upon destruction
            active_.store(false);
            if (worker_.joinable()) {
                worker_.join();
            }
        }

        TimedDataSender(const TimedDataSender&) = delete;
        TimedDataSender& operator=(const TimedDataSender&) = delete;
        TimedDataSender(TimedDataSender&&) = delete;
        TimedDataSender& operator=(TimedDataSender&&) = delete;

    private:
        void loop() {
            using namespace std::chrono;
            auto next_run_time = steady_clock::now() + milliseconds(interval_);

            while (active_.load()) {
                auto now = steady_clock::now();
                if (now >= next_run_time) {
                    // It's time to perform the task
                    task_();

                    // Schedule the next run
                    next_run_time = now + milliseconds(interval_);
                } else {
                    // Sleep for a short while to prevent busy waiting
                    std::this_thread::sleep_for(milliseconds(1));
                }
            }
        }

        std::thread worker_;
        std::atomic<bool> active_;
        unsigned interval_; // The interval between task executions, in milliseconds
        std::function<void()> task_; // The task to be performed
};