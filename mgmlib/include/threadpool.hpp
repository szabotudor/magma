#pragma once
#include <cstddef>
#include <functional>


namespace mgm {
    class ThreadPool {
        struct Data;
        Data* data = nullptr;

        public:
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        /**
         * @brief Construct a new Thread Pool object
         * 
         * @param workers The amount of worker threads to create
         */
        ThreadPool(const size_t workers);

        /**
         * @brief Push a task to the task queue. As soon as a worker is available, it will pick up the task.
         * 
         * @param task The task to push (a callable function that is run once when picked up)
         */
        void push_task(const std::function<void()>& task);

        ~ThreadPool();
    };
}
