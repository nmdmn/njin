#pragma once

#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <format>
#include <chrono>

#include "config.hh"

#ifndef NJIN_LOGGER
#define NJIN_LOGGER 3
#endif

namespace njin {

class Logger {
public:
  enum class Level { TRACE = 1, INFO = 2, WARNING = 3, ERROR = 4 };

  struct Null {
    template <typename T> Null &operator<<(const T &) { return *this; }
  };

  struct Stream {
    Stream(Level level, Logger &logger)
        : level_(level)
        , logger_(logger) {}

    ~Stream() {
      if (moved_)
        return;
      logger_.log(level_, string_stream_.str());
    }

    Stream(Stream &&other)
        : level_(other.level_)
        , logger_(other.logger_)
        , string_stream_(std::move(other.string_stream_)) {
      other.moved_ = true;
    }

    Stream(const Stream &) = delete;
    Stream &operator=(const Stream &) = delete;

    template <typename T> Stream &operator<<(const T &msg) {
      string_stream_ << msg;
      return *this;
    }

    Level level_;
    Logger &logger_;
    bool moved_ = false;
    std::ostringstream string_stream_;
  };

  template <Level L> struct Proxy {
    template <typename T> auto operator<<(const T &msg) const {
      if constexpr (static_cast<int>(L) >= NJIN_LOGGER) {
        Stream stream(L, logger_);
        stream << msg;
        return stream;
      } else {
        return Null{};
      }
    }

    Logger &logger_;
  };

  static Logger &instance() {
    static Logger logger;
    return logger;
  }

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;

  Proxy<Level::TRACE> trace{*this};
  Proxy<Level::INFO> info{*this};
  Proxy<Level::WARNING> warning{*this};
  Proxy<Level::ERROR> error{*this};

private:
  static constexpr std::string_view level_str(Level level) {
    switch (level) {
    case Level::TRACE:
      return "TRACE";
    case Level::INFO:
      return "INFO";
    case Level::WARNING:
      return "WARNING";
    case Level::ERROR:
      return "ERROR";
    }
  }

  Logger() { worker_ = std::thread(&Logger::process, this); }
  ~Logger() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      drained_cond_var_.wait(lock, [this] { return queue_.empty(); });
    }
    is_running_.store(false);
    cond_var_.notify_one();
    worker_.join();
  }

  void process() {
    do {
      decltype(queue_) local;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_var_.wait(lock, [this] { return !queue_.empty() || !is_running_; });
        std::swap(local, queue_);
      }
      while (!local.empty()) {
        std::cout << local.front() << std::endl;
        local.pop();
      }
      drained_cond_var_.notify_one();
    } while (is_running_);
  }

  void log(Level level, std::string_view msg) {
    auto now = std::chrono::system_clock::now();
    std::string entry = std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}", std::chrono::floor<std::chrono::seconds>(now),
                                    level_str(level), msg);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(std::string(entry));
    }
    cond_var_.notify_one();
  }

  std::queue<std::string> queue_;
  std::thread worker_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
  std::condition_variable drained_cond_var_;
  std::atomic<bool> is_running_ = true;
};
} // namespace njin
