#include "http/concurrency/ThreadPool.hpp"

namespace concurrency {

ThreadPool::ThreadPool(std::size_t numThreads) : ring_buffer_(CAPACITY) {
    for(std::size_t i = 0; i < numThreads; i++){
        workers_.emplace_back([this]{
            while(true){
                tasks_available_.acquire();

                if(stop_.load(std::memory_order_acquire)){
                    return;
                }

                std::function<void()> task;

                while(lock_.test_and_set(std::memory_order_acquire)){
                    std::this_thread::yield();
                }

                task = std::move(ring_buffer_[head_]);
                head_ = (head_ + 1) % CAPACITY;
                lock_.clear(std::memory_order_release);

                space_available_.release();

                if(task){
                    task();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool(){
    stop_.store(true, std::memory_order_release);

    for(std::size_t i = 0; i < workers_.size(); ++i){
        tasks_available_.release();
    }

    for(auto& worker: workers_){
        if(worker.joinable()){
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task){
    space_available_.acquire();

    while(lock_.test_and_set(std::memory_order_acquire)){
        std::this_thread::yield();
    }

    ring_buffer_[tail_] = std::move(task);
    tail_ = (tail_ + 1) % CAPACITY;

    lock_.clear(std::memory_order_release);

    tasks_available_.release();
}

} // namespace concurrency