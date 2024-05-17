
// Based on example in https://www.geeksforgeeks.org/thread-pool-in-cpp/

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>

class ThreadPool {
public:
  // Create a thread pool with given number of threads
  ThreadPool(size_t num_threads) {

    // Creating worker threads
    for (size_t i = 0; i < num_threads; ++i) {
      workerThreads.emplace_back([this] {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(mu_queue);

            // Waiting until there is a task to execute or the pool is stopped
            cv_.wait(lock, [this] { return !taskQueue.empty() || stop_; });

            // Exit the thread in case the pool is stopped and there are no tasks
            if (stop_ && taskQueue.empty()) {
              return;
            }

            // Get the next task from the queue
            task = std::move(taskQueue.front());
            taskQueue.pop();
          }

          task();
        }
      });
    }
  }

  // Destructor to stop the thread pool
  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mu_queue);
      stop_ = true;
    }

    // Notify all threads
    cv_.notify_all();

    // Joining all worker threads to ensure they have
    // completed their tasks
    for (auto &thread : workerThreads) {
      thread.join();
    }
  }

  // Wait until all tasks assigned to the threads have been finished
  void waitUntilDone() {
    while (true) {
      {
        std::lock_guard<std::mutex> guard(mu_queue);
        if (taskQueue.empty()) {
          return;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }

  // Enqueue task for execution by the thread pool
  void enqueue(std::function<void()> task) {
    {
      std::lock_guard<std::mutex> lock(mu_queue);
      taskQueue.emplace(move(task));
    }
    cv_.notify_one();
  }

private:
  std::vector<std::thread> workerThreads;
  std::queue<std::function<void()>> taskQueue;
  std::mutex mu_queue;

  // Condition variable to signal changes in the state of the tasks queue
  std::condition_variable cv_;

  // Flag to indicate whether the thread pool should stop
  bool stop_ = false;
};
