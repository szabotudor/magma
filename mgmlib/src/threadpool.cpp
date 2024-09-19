#include "threadpool.hpp"
#include "logging.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>


namespace mgm {
    struct ThreadPool::Data {
        struct Worker {
            std::function<void()> task{};
            std::thread thread{};
            std::mutex mutex{};
            std::condition_variable cv{};
            bool has_task = false;
            bool alive = true;

            Worker() = default;
            Worker(Worker&& w) {
                if (this == &w)
                    return;

                std::unique_lock<std::mutex> lock(w.mutex);
                w.cv.wait(lock, [&w] { return !w.has_task; });

                task = std::move(w.task);
                thread = std::move(w.thread);
                has_task = w.has_task;
                alive = w.alive;
                w.has_task = false;
                w.alive = false;
            }
        };
        std::vector<Worker> workers{};

        std::vector<std::function<void()>> queue{};

        std::thread main{};
        std::mutex mutex{};
        std::atomic_bool running{};
        std::atomic_size_t avail_workers{0};
    };

    ThreadPool::ThreadPool(const size_t num_workers) : data{new Data{}} {
        data->workers.resize(num_workers);
        data->avail_workers = num_workers;
        for (auto& worker : data->workers) {
            worker.thread = std::thread([&worker, this] {
                while (true) {
                    std::unique_lock<std::mutex> lock(worker.mutex);
                    worker.cv.wait(lock, [&worker] { return worker.has_task || !worker.alive; });
                    if (!worker.alive)
                        return;

                    worker.task();

                    worker.task = {};
                    worker.has_task = false;
                    data->avail_workers++;

                    lock.unlock();
                    worker.cv.notify_all();
                }
            });
        }

        data->main = std::thread([this] {
            data->running = true;
            auto start = std::chrono::high_resolution_clock::now();
            while (true) {
                if (!data->running)
                    break;
                data->mutex.lock();
                const auto now = std::chrono::high_resolution_clock::now();
                const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                if (millis > 1000 && !data->queue.empty()) {
                    Logging{"ThreadPool"}.warning("Long running thread possibly blocking tasks in queue");
                    start = now;
                }

                if (data->queue.empty()) {
                    data->mutex.unlock();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }

                if (data->avail_workers == 0) {
                    data->mutex.unlock();
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }

                start = std::chrono::high_resolution_clock::now();
                const auto task = data->queue.back();
                data->queue.pop_back();
                data->mutex.unlock();

                for (auto& worker : data->workers) {
                    std::unique_lock<std::mutex> lock(worker.mutex);
                    if (!worker.has_task) {
                        worker.task = task;
                        worker.has_task = true;
                        data->avail_workers--;
                        lock.unlock();
                        worker.cv.notify_all();
                        break;
                    }
                }
            }
        });
    }

    void ThreadPool::push_task(const std::function<void()>& task) {
        data->mutex.lock();
        data->queue.emplace_back(task);
        data->mutex.unlock();
    }

    ThreadPool::~ThreadPool() {
        data->mutex.lock();
        data->queue.clear();
        data->running = false;
        data->mutex.unlock();
        data->main.join();

        for (auto& worker : data->workers) {
            {
                std::unique_lock<std::mutex> lock(worker.mutex);
                worker.alive = false;
                worker.cv.notify_one();
            }
            worker.thread.join();
        }
        delete data;
    }
}
