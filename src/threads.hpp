#ifndef THREADS_HPP
#define THREADS_HPP

#include <vector>
#include <thread>

class Threads {
    public:
        template <typename Callable>
        void addThread(Callable&& func) {
            threads.emplace_back(std::thread(std::forward<Callable>(func)));
        }
        void joinAll() {
            for (auto& thread : threads) 
            {
                if (thread.joinable()) 
                {
                    thread.join();
                }
            }
        }
        // Destructor to ensure all threads are joined
        ~Threads() {
            joinAll();
        }
    ;
    private:
        std::vector<std::thread> threads;
    ;
};

#endif // THREADS_HPP