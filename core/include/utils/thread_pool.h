#ifndef UTILS_THREAD_POOL_H_
#define UTILS_THREAD_POOL_H_

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <iostream>
#include <unistd.h>

class ThreadPool {
 public:

  ThreadPool(int threads)
      : terminate_(false),
        stopped_(false) {
    for (int i = 0; i < threads; i++) {
      thread_pool_.emplace_back(std::thread(&ThreadPool::Invoke, this));
    }
  }

  ~ThreadPool() {
    if (!stopped_) {
      ShutDown();
    }
  }

  void Enqueue(std::function<void()> f) {
    {
      std::unique_lock<std::mutex> lock(tasks_mutex_);
      tasks_.push(f);
    }
    condition_.notify_one();
  }

  void ShutDown() {
    {
      std::unique_lock<std::mutex> lock(tasks_mutex_);
      terminate_ = true;
    }

    condition_.notify_all();
    for (std::thread &thread : thread_pool_) {
      thread.join();
    }

    thread_pool_.clear();
    stopped_ = true;
  }

 private:
  void Invoke() {
    std::function<void()> task;
    while (true) {
      {
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        condition_.wait(lock, [this] {return !tasks_.empty() || terminate_;});
        if (terminate_ && tasks_.empty()) {
          return;
        }

        task = tasks_.front();
        tasks_.pop();
      }

      task();
    }
  }

  std::vector<std::thread> thread_pool_;
  std::queue<std::function<void()>> tasks_;
  std::mutex tasks_mutex_;
  std::condition_variable condition_;
  bool terminate_;
  bool stopped_;

};

#endif
