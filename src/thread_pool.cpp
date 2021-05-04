#include "thread_pool.hpp"

namespace myrt {
  thread_pool::thread_pool(unsigned concurrency)
  {
    for (unsigned i = 0; i < concurrency; ++i)
    {
      m_threads.push_back(std::jthread(&thread_loop, this));
    }
  }
  thread_pool::~thread_pool()
  {
    for (auto& thread : m_threads)
      thread.request_stop();
    m_wait_condition.notify_all();
  }
  void thread_pool::run_detached(std::function<void()> job)
  {
    std::unique_lock<std::mutex> lock(m_jobs_mutex);
    m_jobs.push_back(std::move(job));
    m_wait_condition.notify_one();
  }
  unsigned thread_pool::concurrency() const
  {
    return static_cast<unsigned>(m_threads.size());
  }
  void thread_pool::thread_loop(std::stop_token stop_token, thread_pool* self)
  {
    while (!stop_token.stop_requested()) {

      std::unique_lock<std::mutex> lock(self->m_jobs_mutex);

      do {
       self->m_wait_condition.wait(lock, [&] { return stop_token.stop_requested() || !self->m_jobs.empty(); });
      } while (!stop_token.stop_requested() && self->m_jobs.empty());

      if (!stop_token.stop_requested())
      {
        auto const fun = std::move(self->m_jobs.front());
        self->m_jobs.pop_front();
        lock.unlock();

        fun();

        lock.lock();
      }
    }
  }
}