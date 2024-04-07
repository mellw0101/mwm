/* #include <future>
#include <mutex>
#include <queue> */
#ifndef THREAD__HPP
#define THREAD__HPP

#include <thread>
#include <vector>
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
// #include <optional>

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
                }
                else {
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

/* #include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool {
    public:
        ThreadPool(size_t threads) : stop(false) {
            for(size_t i = 0; i < threads; ++i) {
                workers.emplace_back([this] {
                    while(true) {
                        std::function<void()> task;
                        {
                            std::unique_copy<std::mutex> lock(this->queueMutex);
                            this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                            if(this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        template<class F, class... Args>
        auto enqueue(F&& f, Args&&... args) 
            -> std::future<typename std::result_of<F(Args...)>::type> {
            using return_type = typename std::result_of<F(Args...)>::type;
            auto task = std::make_shared< std::packaged_task<return_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );   
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if(stop)
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                tasks.emplace([task](){ (*task)(); });
            }
            condition.notify_one();
            return res;
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                stop = true;
            }
            condition.notify_all();
            for(std::thread &worker: workers)
                worker.join();
        }

    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queueMutex;
        std::condition_variable condition;
        bool stop;
}; */


#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#define enqueueT(__Name, __Pool, ...) \
    std::future<void> __Name = __Pool.enqueue(__VA_ARGS__)
template<typename ThreadPoolType, typename Func, typename... Args>
auto enqueueTask(ThreadPoolType& pool, Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
    return pool.enqueue(std::forward<Func>(func), std::forward<Args>(args)...);
}

class ThreadPool {
    public:
        ThreadPool(size_t threads) : stop(false) {
            for(size_t i = 0; i < threads; ++i) {
                workers.emplace_back([this] {
                    while(true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queueMutex);
                            this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                            if(this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();

                        } task();

                    }

                });

            }
                
        }

        template<class Callback, class... Args>
        auto enqueue(Callback&& f, Args&&... args)
        -> std::future<typename std::invoke_result<Callback, Args...>::type> {
            using return_type = typename std::invoke_result<Callback, Args...>::type;

            auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<Callback>(f), std::forward<Args>(args)...)

            );

            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                if(stop)
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                tasks.emplace([task](){ (*task)(); });
            
            } condition.notify_one();

            return res;

        }
        
        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                stop = true;
            
            } condition.notify_all();

            for(std::thread &worker: workers) {
                worker.join();

            }

        }

    private:
        vector<thread> workers;
        queue<function<void()>> tasks;
        mutex queueMutex;
        condition_variable condition;
        bool stop;

};
/** USAGE: ->
    int main() {
        // Create a ThreadPool with 4 worker threads
        ThreadPool pool(4);

        // Enqueue some tasks into the pool
        std::future<void> result1 = pool.enqueue(workFunction, 1);
        std::future<void> result2 = pool.enqueue(workFunction, 2);

        // Optionally, wait for a specific task to complete
        result1.wait();
        result2.wait();

        // The ThreadPool will automatically wait for all enqueued tasks to complete
        // upon destruction, so an explicit wait is not strictly necessary unless
        // you need to synchronize with tasks' completion.

        return 0;
    }

    void foo() {
        ThreadPool pool(4); // Assuming ThreadPool is defined and instantiated

        auto futureResult = enqueueTask(pool, [](int x) -> int {
            return x * 2;
        }, 10);

        int result = futureResult.get();
        std::cout << "The result is: " << result << std::endl;
    }
*/

class ThreadWrapper {
    public:
        ThreadWrapper() : running(false) {}

        ~ThreadWrapper() {
            stop();
        }

        // Delete copy constructor and assignment operator
        ThreadWrapper(const ThreadWrapper&) = delete;
        ThreadWrapper& operator=(const ThreadWrapper&) = delete;

        // Start the thread with a new task
        template<typename Callable, typename... Args>
        void start(Callable&& task, Args&&... args) {
            // Ensure any running thread is stopped before starting a new one
            stop();

            // Set the running flag and start the new thread
            running = true;
            thread = std::thread([this, task = std::function<void()>(std::bind(std::forward<Callable>(task), std::forward<Args>(args)...))] {
                task();
                running = false;
            });
        }

        // Stop the thread
        void stop() {
            if (running) {
                running = false; // Set running flag to false, allowing the thread to exit if it checks this flag
                if (thread.joinable()) {
                    thread.join(); // Wait for the thread to finish execution
                }
            }
        }

        // Check if the thread is running
        bool isRunning() const {
            return running;
        }

    private:
        std::thread thread;
        std::atomic<bool> running;
};

#include "Log.hpp"

class AsyncWrapper_first {
    public:
        AsyncWrapper_first() = default;

        // Delete copy semantics
        AsyncWrapper_first(const AsyncWrapper_first&) = delete;
        AsyncWrapper_first& operator=(const AsyncWrapper_first&) = delete;

        // Allow move semantics
        AsyncWrapper_first(AsyncWrapper_first&&) = default;
        AsyncWrapper_first& operator=(AsyncWrapper_first&&) = default;

        // Start an asynchronous task
        template<typename Callable, typename... Args>
        void start(Callable&& task, Args&&... args) {
            // Ensure any previous task is not pending
            if (future.valid()) {
                try {
                    future.get(); // Attempt to retrieve the result, if any, or wait for completion
                } catch (const std::exception& e) {
                    // Handle or log exceptions from the previous task
                    loutE << "Exception from previous task: " << e.what() << loutEND;
                }
            }

            // Launch a new asynchronous task
            future = std::async(std::launch::async, std::forward<Callable>(task), std::forward<Args>(args)...);
        }

        // Attempt to retrieve the result of the asynchronous task, waiting if necessary
        template<typename ResultType>
        ResultType get() {
            if (!future.valid()) {
                throw std::runtime_error("No valid future available. Did you start a task?");
            }
            return future.get(); // This will wait for the task to complete if it hasn't already
        }

    private:
        std::future<void> future;

};

enum class TaskState {
    Idle,
    Waiting,
    Running,
    Completed,
    Exception
};

class AsyncWrapper {
    public:
        AsyncWrapper() : state(TaskState::Idle) {}

        // Prevent copy operations
        AsyncWrapper(const AsyncWrapper&) = delete;
        AsyncWrapper& operator=(const AsyncWrapper&) = delete;

        // Enable move operations
        AsyncWrapper(AsyncWrapper&&) = delete;
        AsyncWrapper& operator=(AsyncWrapper &&) = delete;

        template<typename Callable, typename... Args>
        void start(Callable&& task, Args&&... args) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (state != TaskState::Idle) {
                    throw std::runtime_error("Task is already set or running.");
                }
                // Set the task but don't start it yet
                this->task = std::bind(std::forward<Callable>(task), std::forward<Args>(args)...);
                state = TaskState::Waiting;
            }
        }

        // Call this method to start the task
        void trigger() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (state != TaskState::Waiting) {
                    throw std::runtime_error("No task is waiting to be triggered.");
                }
                state = TaskState::Running;
            }
            condition.notify_one(); // Signal the condition variable to start the task

            future = std::async(std::launch::async, [this]() {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this]{ return state == TaskState::Running; }); // Wait to be triggered
                try {
                    task(); // Execute the task
                    state = TaskState::Completed;
                } catch (...) {
                    state = TaskState::Exception;
                }
            });
        }

        TaskState getState() const {
            return state;
        }

        bool isFinished() const {
            return state == TaskState::Completed || state == TaskState::Exception;
        }

    private:
        std::future<void> future;
        std::function<void()> task;
        std::mutex mutex;
        std::condition_variable condition;
        std::atomic<TaskState> state;
};

class root_thread {
    private:


    public:
        vector<thread> threads;

};

#endif