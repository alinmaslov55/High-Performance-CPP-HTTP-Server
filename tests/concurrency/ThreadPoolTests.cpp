#include <gtest/gtest.h>
#include "http/concurrency/ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace concurrency_tests {

using namespace concurrency;

TEST(ThreadPoolTest, ExecutesSingleTask) {
    std::atomic<bool> task_executed{false};
    
    {
        ThreadPool pool(2);
        
        pool.enqueue([&task_executed]() {
            task_executed = true;
        });

        while (!task_executed.load()) {
            std::this_thread::yield();
        }
    }

    EXPECT_TRUE(task_executed.load());
}

TEST(ThreadPoolTest, ExecutesMultipleTasksConcurrently) {
    std::atomic<int> counter{0};
    const int num_tasks = 1000;
    
    {
        ThreadPool pool(4);
        
        for (int i = 0; i < num_tasks; ++i) {
            pool.enqueue([&counter]() {
                counter++; 
            });
        }

        while (counter.load() < num_tasks) {
            std::this_thread::yield();
        }
    }
    
    EXPECT_EQ(counter.load(), num_tasks);
}

TEST(ThreadPoolTest, HandlesHeavyWorkloads) {
    std::atomic<int> completed_tasks{0};
    const int num_tasks = 50;
    
    {
        ThreadPool pool(4);
        
        for (int i = 0; i < num_tasks; ++i) {
            pool.enqueue([&completed_tasks]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                completed_tasks++;
            });
        }

        while (completed_tasks.load() < num_tasks) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    } 
    
    EXPECT_EQ(completed_tasks.load(), num_tasks);
}

} // namespace concurrency_tests