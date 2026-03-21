#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <vector>
#include <thread>
#include <atomic>
#include <future>
#include "logger.hh"

TEST_CASE("Logger is a singleton", "[logger]") {
  auto &a = njin::Logger::instance();
  auto &b = njin::Logger::instance();
  REQUIRE(&a == &b);
}

TEST_CASE("Logger accepts messages from multiple threads", "[logger]") {
  auto run = GENERATE(range(1, 1001));
  (void)run;

  auto &logger = njin::Logger::instance();
  std::atomic<int> count = 0;

  constexpr int NUM_ITERATIONS = 100;
  constexpr int NUM_THREADS = 5;
  auto test = std::async(std::launch::async, [&] {
    auto worker = [&](int id) {
      for (int i = 0; i < NUM_ITERATIONS; ++i) {
        logger.info << "thread " << id << " msg " << i;
        ++count;
      }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
      threads.emplace_back(worker, i);
    }

    std::ranges::for_each(threads, &std::thread::join);
  });

  REQUIRE(test.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  REQUIRE(count == NUM_ITERATIONS * NUM_THREADS);
}
