#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace concurrency {

constexpr int DEFAULT_THREADS = 4;

class ThreadPool {
public:
    explicit ThreadPool(std::size_t numThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    void enqueue(std::function<void()> task);

    [[nodiscard]]
    std::size_t getPendingTaskCount() const {
        return pending_tasks_.load(std::memory_order_relaxed);
    }

private:
    static constexpr std::size_t CAPACITY = 1024;
    std::vector<std::function<void()>> ring_buffer_;
    std::vector<std::thread> workers_;
    std::size_t head_{0};
    std::size_t tail_{0};

    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

    std::counting_semaphore<> tasks_available_{0};
    std::counting_semaphore<> space_available_{CAPACITY};

    std::atomic<bool> stop_{false};
    std::atomic<std::size_t> pending_tasks_{0};
};

} // namespace concurrency

#endif // THREAD_POOL_HPP