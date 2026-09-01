#include "http/concurrency/ThreadPool.hpp"

namespace concurrency {

ThreadPool::ThreadPool(std::size_t numThreads) : stop_(false){
    for(std::size_t i = 0; i < numThreads; i++){
        workers_.emplace_back([this]{
            while(true){
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);

                    this->condition_.wait(lock, [this]{
                        return this->stop_ || !this->tasks_.empty();
                    });

                    if(this->stop_ && this->tasks_.empty()){
                        return;
                    }

                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }

    condition_.notify_all();

    for(auto& worker: workers_){
        worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task){
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace(std::move(task));
    }

    condition_.notify_one();
}

} // namespace concurrency