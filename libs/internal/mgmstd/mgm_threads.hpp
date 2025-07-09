#pragma once
#include <mutex>
#include <thread>


namespace mgm {
    using MThread = std::thread;

    using MMutex = std::mutex;
} // namespace mgm
