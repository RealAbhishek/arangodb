////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2024 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Business Source License 1.1 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     https://github.com/arangodb/arangodb/blob/devel/LICENSE
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Julia Volmer
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Async/Registry/registry_variable.h"
#include "Async/Registry/Metrics.h"
#include "Basics/FutureSharedLock.h"
#include "RestServer/arangod.h"
#include "Scheduler/SchedulerFeature.h"

namespace arangodb::async_registry {

class Feature final : public ArangodFeature {
 private:
  static auto create_metrics(arangodb::metrics::MetricsFeature& metrics_feature)
      -> std::shared_ptr<const Metrics>;
  struct SchedulerWrapper {
    using WorkHandle = Scheduler::WorkHandle;
    template<typename F>
    void queue(F&& fn) {
      SchedulerFeature::SCHEDULER->queue(RequestLane::CLUSTER_INTERNAL,
                                         std::forward<F>(fn));
    }
    template<typename F>
    WorkHandle queueDelayed(F&& fn, std::chrono::milliseconds timeout) {
      return SchedulerFeature::SCHEDULER->queueDelayed(
          "rocksdb-meta-collection-lock-timeout", RequestLane::CLUSTER_INTERNAL,
          timeout, std::forward<F>(fn));
    }
  };

 public:
  static constexpr std::string_view name() { return "Coroutines"; }
  auto asyncLock() -> futures::Future<
      futures::FutureSharedLock<SchedulerWrapper>::LockGuard>;

  Feature(Server& server);

  ~Feature();

  void start() override final;
  void stop() override final;
  void collectOptions(std::shared_ptr<options::ProgramOptions>) override final;

 private:
  struct Options {
    size_t gc_timeout{1};
  };
  Options _options;

  std::shared_ptr<const Metrics> metrics;

  struct PromiseCleanupThread;
  std::shared_ptr<PromiseCleanupThread> _cleanupThread;

  SchedulerWrapper _schedulerWrapper;
  futures::FutureSharedLock<SchedulerWrapper> _async_mutex;
};

}  // namespace arangodb::async_registry
