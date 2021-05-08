#pragma once

#include <thread>
#include <future>
#include <vector>
#include <functional>
#include <mutex>
#include <deque>
#include <experimental/generator>

namespace myrt {
  template<typename Func, typename... Args>
  concept callable = requires(Func func, Args... args) { func(args...); };

  template<typename Func, typename Type>
  concept async_loader = requires(Func func) { { func() }->std::convertible_to<Type>; };

  template<typename T>
  concept is_atomic = std::is_trivially_copyable_v<T> && std::is_copy_constructible_v<T> &&
    std::is_move_constructible_v<T> && std::is_copy_assignable_v<T> && std::is_move_assignable_v<T>;

  template<typename T>
  concept copyable_or_movable = std::copyable<T> || std::movable<T>;

  template<typename T>
  concept iterable = requires(T t) { begin(t); end(t); };

  template<typename T>
  consteval auto iterable_type_get() {
    if constexpr (iterable<T>)
      return std::decay_t<decltype(*begin(std::declval<T>()))>{};
  }

  class thread_pool {
  public:
    [[nodiscard]] thread_pool(unsigned concurrency = std::thread::hardware_concurrency());
    ~thread_pool();

    template<callable Func>
    [[nodiscard]] auto run_async(Func&& func);

    void run_detached(std::function<void()> job);

    [[nodiscard]] unsigned concurrency() const;

  private:
    static void thread_loop(std::stop_token stop_token, thread_pool* self);

    std::vector<std::jthread> m_threads;
    std::deque<std::function<void()>> m_jobs;
    std::mutex m_jobs_mutex;
    std::condition_variable m_wait_condition;
  };

  template<copyable_or_movable T>
  struct async_resource {
    struct empty_mutex {};
    struct empty_lock {};

    using iterable_type = decltype(iterable_type_get<T>());
    using value_type = std::conditional_t<is_atomic<T>, std::atomic<T>, T>;
    using mutex_type = std::conditional_t<is_atomic<T>, empty_mutex, std::mutex>;
    using lock_type = std::conditional_t<is_atomic<T>, empty_lock, std::unique_lock<std::mutex>>;

    [[nodiscard]] async_resource() = default;
    [[nodiscard]] async_resource(T&& value) requires std::movable<T> : m_value(std::move(value)) {}
    [[nodiscard]] async_resource(T const& value) requires std::copyable<T> : m_value(value) {}
    template<async_loader<T> Func>
    [[nodiscard]] async_resource(thread_pool& pool, Func&& loader) {
      load_resource(pool, std::forward<Func>(loader));
    }

    [[nodiscard]] std::experimental::generator<iterable_type const*> iterate() const requires iterable<T> {
      auto const lock = make_lock();
      for (iterable_type const& item : m_value)
        co_yield &item;
    }

    [[nodiscard]] std::experimental::generator<iterable_type*> iterate() requires iterable<T> {
      auto const lock = make_lock();
      for (iterable_type& item : m_value)
        co_yield &item;
    }

    template<callable<T const&> ApplyFun>
    void current(ApplyFun&& apply) const
    {
      auto const lock = make_lock();
      apply(static_cast<T>(m_value));
    }

    [[nodiscard]] T&& extract_current() requires std::movable<T> {
      auto const lock = make_lock();
      return std::move(m_value);
    }

    template<async_loader<T> Func>
    bool load_resource(thread_pool& pool, Func&& loader)
    {
      if (!is_ready()) return false;
      m_current_process = pool.run_async([ld = std::forward<Func>(loader), this]{
        auto value = ld();
        auto const lock = make_lock();
        if constexpr (std::movable<T>)
          m_value = std::move(value);
        else if constexpr (std::copyable<T>)
          m_value = value;
        });
      return true;
    }

    void wait() const {
      m_current_process.wait();
    }

    bool is_ready()
    {
      return !m_current_process.valid() ||
        m_current_process.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

  private:
    lock_type make_lock() const {
      if constexpr (is_atomic<T>)
        return empty_lock{};
      else
        return std::unique_lock<std::mutex>(m_value_mutex);
    }

    value_type m_value;
    mutable mutex_type m_value_mutex;
    std::future<void> m_current_process;
  };

  template<callable Func>
  auto thread_pool::run_async(Func&& func)
  {
    using result_type = decltype(func());
    auto promise = std::make_shared<std::promise<result_type>>();
    std::future<result_type> future = promise->get_future();

    std::unique_lock<std::mutex> lock(m_jobs_mutex);
    m_jobs.push_back([p = std::move(promise), f = std::forward<Func&&>(func)]() mutable{
      try {
        if constexpr (std::same_as<result_type, void>)
        {
          f();
          p->set_value();
        }
        else
        {
          p->set_value(f());
        }
      }
      catch (...)
      {
        p->set_exception(std::current_exception());
      }
    });
    m_wait_condition.notify_one();
    return future;
  }
}