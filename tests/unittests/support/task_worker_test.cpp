/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "srsran/support/executors/task_worker.h"
#include "srsran/support/executors/task_worker_pool.h"
#include <future>
#include <gtest/gtest.h>

using namespace srsran;

TEST(task_worker, correct_initialization)
{
  task_worker worker{"WORKER", 1024};
  ASSERT_EQ(std::string(worker.worker_name()), "WORKER");
  ASSERT_EQ(worker.max_pending_tasks(), 1024);
  ASSERT_EQ(worker.nof_pending_tasks(), 0);
}

TEST(task_worker, multiple_stops_can_be_called)
{
  task_worker worker{"WORKER", 1024};
  worker.stop();
  worker.stop();
}

TEST(task_worker, single_pushed_task_is_run)
{
  task_worker           worker{"WORKER", 1024};
  std::atomic<uint32_t> count{0};
  ASSERT_TRUE(worker.push_task([&count]() { count++; }));
  worker.wait_pending_tasks();
  ASSERT_EQ(count, 1);
}

TEST(task_worker_pool, correct_initialization)
{
  task_worker_pool pool{4, 128, "POOL"};
  ASSERT_EQ(pool.nof_workers(), 4);
  ASSERT_EQ(pool.nof_pending_tasks(), 0);
}

TEST(task_worker_pool, worker_pool_runs_single_task)
{
  task_worker_pool pool{4, 128, "POOL"};

  std::promise<void> p;
  std::future<void>  f = p.get_future();
  pool.push_task([&p]() {
    p.set_value();
    fmt::print("Finished in {}\n", this_thread_name());
  });
  f.get();
}

TEST(spsc_task_worker_test, correct_initialization)
{
  general_task_worker<concurrent_queue_policy::lockfree_spsc, concurrent_queue_wait_policy::sleep> worker{
      "WORKER", 128, std::chrono::microseconds{100}};

  ASSERT_EQ(std::string(worker.worker_name()), "WORKER");
  ASSERT_EQ(worker.max_pending_tasks(), 128);
  ASSERT_EQ(worker.nof_pending_tasks(), 0);
}
TEST(spsc_task_worker_test, single_pushed_task_is_run)
{
  general_task_worker<concurrent_queue_policy::lockfree_spsc, concurrent_queue_wait_policy::sleep> worker{
      "WORKER", 128, std::chrono::microseconds{100}};

  std::atomic<uint32_t> count{0};
  ASSERT_TRUE(worker.push_task([&count]() { count++; }));
  worker.wait_pending_tasks();
  ASSERT_EQ(count, 1);
}
