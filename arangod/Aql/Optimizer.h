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
/// @author Max Neunhoeffer
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Aql/ExecutionPlan.h"
#include "Basics/ResourceUsage.h"

#include <velocypack/Builder.h>

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace arangodb::aql {
struct OptimizerRule;
struct QueryOptions;

class Optimizer {
 private:
  /// @brief this stored the positions of rules in OptimizerRulesFeature::_rules
  using RuleDatabase = std::vector<int>;

  /// @brief the following struct keeps a container of ExecutionPlan objects
  /// and has some automatic convenience functions.
  struct PlanList {
    using Entry =
        std::pair<std::unique_ptr<ExecutionPlan>, RuleDatabase::iterator>;

    std::deque<Entry, ResourceUsageAllocator<Entry, ResourceMonitor>> list;

    explicit PlanList(ResourceMonitor& monitor) : list(monitor) {}

    /// @brief get number of plans contained
    size_t size() const noexcept { return list.size(); }

    /// @brief check if empty
    bool empty() const noexcept { return list.empty(); }

    /// @brief pop the first one
    Entry pop_front() {
      auto p = std::move(list.front());
      list.pop_front();
      return p;
    }

    /// @brief push_back
    void push_back(std::unique_ptr<ExecutionPlan> p,
                   RuleDatabase::iterator rule) {
      list.push_back({std::move(p), rule});
    }

    /// @brief swaps the two lists
    void swap(PlanList& b) noexcept { list.swap(b.list); }

    /// @brief clear, deletes all plans contained
    void clear() noexcept { list.clear(); }
  };

 public:
  /// @brief optimizer statistics
  struct Stats {
    int64_t rulesExecuted = 0;
    int64_t rulesSkipped = 0;
    int64_t plansCreated = 1;  // 1 for the initial plan

    /// @brief map with execution times per rule, only populated in
    /// case tracing is enabled, a nullptr otherwise!
    std::unique_ptr<std::unordered_map<int, double>> executionTimes;

    void toVelocyPack(velocypack::Builder& b) const;
    static void toVelocyPackForCachedPlan(velocypack::Builder& b);
  };

  /// @brief constructor, this will initialize the rules database
  /// the .cpp file includes Aql/OptimizerRules.h
  /// and add all methods there to the rules database
  explicit Optimizer(ResourceMonitor& resourceMonitor, size_t maxNumberOfPlans);

  /// @brief disable rules in the given plan, using the predicate function
  void disableRules(ExecutionPlan* plan,
                    std::function<bool(OptimizerRule const&)> const& predicate);

  /// @brief do the optimization, this does the optimization, the resulting
  /// plans are all estimated, sorted by that estimate and can then be got
  /// by getPlans, until the next initialize is called. Note that the optimizer
  /// object takes ownership of the execution plan and will delete it
  /// automatically on destruction. It will also have ownership of all the
  /// newly created plans it recalls and will automatically delete them.
  /// If you need to extract the plans from the optimizer use stealBest or
  /// stealPlans.
  void createPlans(std::unique_ptr<ExecutionPlan> p,
                   QueryOptions const& queryOptions, bool estimateAllPlans);

  /// @brief add a plan to the optimizer
  void addPlan(std::unique_ptr<ExecutionPlan>, OptimizerRule const&,
               bool wasModified);

  /// @brief add a plan to the optimizer and makes it rerun the current rule
  /// again
  void addPlanAndRerun(std::unique_ptr<ExecutionPlan>, OptimizerRule const&,
                       bool wasModified);

  /// @brief getPlans, ownership of the plans remains with the optimizer
  auto const& getPlans() noexcept { return _plans.list; }

  /// @brief stealBest, ownership of the plan is handed over to the caller,
  /// all other plans are deleted
  std::unique_ptr<ExecutionPlan> stealBest() {
    if (_plans.empty()) {
      return nullptr;
    }
    auto res = std::move(_plans.list.front());
    _plans.list.clear();
    return std::move(res.first);
  }

  bool runOnlyRequiredRules() const noexcept;

  /// @brief numberOfPlans, returns the current number of plans in the system
  /// this should be called from rules, it will consider those that the
  /// current rules has already added
  size_t numberOfPlans() const noexcept {
    return _plans.size() + _newPlans.size() + 1;
  }

  void initializeRules(ExecutionPlan* plan, QueryOptions const& queryOptions);

  void toVelocyPack(velocypack::Builder& b) const;

 private:
  /// @brief disable a specific rule
  void disableRule(ExecutionPlan* plan, int level);

  /// @brief disable a specific rule, by name
  void disableRule(ExecutionPlan* plan, std::string_view name);

  /// @brief enable a specific rule
  void enableRule(ExecutionPlan* plan, int level);

  /// @brief enable a specific rule, by name
  void enableRule(ExecutionPlan* plan, std::string_view name);

  /// @brief adds a plan, internal worker method
  void addPlanInternal(std::unique_ptr<ExecutionPlan> plan,
                       OptimizerRule const& rule, bool wasModified,
                       RuleDatabase::iterator const& nextRule);

  void finalizePlans();

  void checkForcedIndexHints();

  void estimateCosts(QueryOptions const& queryOptions, bool estimateAllPlans);

  /// @brief optimizer statistics
  Stats _stats;

  /// @brief the current set of plans to be optimized
  PlanList _plans;

  /// @brief current list of plans (while applying optimizer rules)
  PlanList _newPlans;

  /// @brief the rule that is currently getting applied
  /// (while applying optimizer rules in createPlans)
  RuleDatabase::iterator _currentRule;

  /// @brief list of optimizer rules to be applied
  RuleDatabase _rules;

  /// @brief maximal number of plans to produce
  size_t const _maxNumberOfPlans;

  /// @brief run only the required optimizer rules
  bool _runOnlyRequiredRules;
};

}  // namespace arangodb::aql
