#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
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
  auto run = GENERATE(range(1, 10001));
  (void)run;

  auto &logger = njin::Logger::instance();
  std::atomic<int> count = 0;

  auto test = std::async(std::launch::async, [&] {
    auto worker = [&](int id) {
      for (int i = 0; i < 10; ++i) {
        logger.info << "thread " << id << " msg " << i;
        ++count;
      }
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);

    t1.join();
    t2.join();
    t3.join();
  });

  REQUIRE(test.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
  REQUIRE(count == 30);
}
