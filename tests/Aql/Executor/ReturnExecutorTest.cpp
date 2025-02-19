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
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"

#include "AqlExecutorTestCase.h"

#include "Mocks/Servers.h"

#include "Aql/AqlItemBlock.h"
#include "Aql/ExecutionBlockImpl.h"
#include "Aql/ExecutionEngine.h"
#include "Aql/Executor/ReturnExecutor.h"
#include "Aql/InputAqlItemRow.h"
#include "Aql/RegisterInfos.h"
#include "Aql/SingleRowFetcher.h"
#include "Basics/ResourceUsage.h"

#include <velocypack/Builder.h>

using namespace arangodb;
using namespace arangodb::aql;

namespace arangodb {
namespace tests {
namespace aql {

using ReturnExecutorParamType = std::tuple<SplitType, bool>;

class ReturnExecutorTest
    : public AqlExecutorTestCaseWithParam<ReturnExecutorParamType> {
 protected:
  auto getSplit() -> SplitType {
    auto [split, unused] = GetParam();
    return split;
  }

  auto doCount() -> bool {
    auto [unused, doCount] = GetParam();
    return doCount;
  }

  auto getCountStats(size_t nr) -> ExecutionStats {
    ExecutionStats stats;
    if (doCount()) {
      stats.count = nr;
    }
    return stats;
  }
};

INSTANTIATE_TEST_CASE_P(
    ReturnExecutor, ReturnExecutorTest,
    ::testing::Combine(::testing::Values(splitIntoBlocks<2, 3>,
                                         splitIntoBlocks<3, 4>, splitStep<1>,
                                         splitStep<2>),
                       ::testing::Bool()));

/*******
 *  Start test suite
 ******/

/**
 * @brief Test the most basic query.
 *        We have an unlimited produce call
 *        And the data is in register 0 => we expect it to
 *        be passed through.
 */

TEST_P(ReturnExecutorTest, returns_all_from_upstream) {
  RegisterInfos registerInfos(RegIdSet{0}, RegIdSet{0}, 1, 1, RegIdFlatSet{},
                              RegIdFlatSetStack{{}});
  ReturnExecutorInfos executorInfos(0 /*input register*/, doCount());
  AqlCall call{};  // unlimited produce
  makeExecutorTestHelper()
      .addConsumer<ReturnExecutor>(std::move(registerInfos),
                                   std::move(executorInfos),
                                   ExecutionNode::RETURN)
      .setInputValueList(1, 2, 5, 2, 1, 5, 7, 1)
      .setInputSplitType(getSplit())
      .setCall(call)
      .expectOutput({0}, {{1}, {2}, {5}, {2}, {1}, {5}, {7}, {1}})
      .expectSkipped(0)
      .expectedState(ExecutionState::DONE)
      .expectedStats(getCountStats(8))
      .run();
}

TEST_P(ReturnExecutorTest, handle_soft_limit) {
  RegisterInfos registerInfos(RegIdSet{0}, RegIdSet{0}, 1, 1, RegIdFlatSet{},
                              RegIdFlatSetStack{{}});
  ReturnExecutorInfos executorInfos(0 /*input register*/, doCount());
  AqlCall call{};
  call.softLimit = 3u;
  makeExecutorTestHelper()
      .addConsumer<ReturnExecutor>(std::move(registerInfos),
                                   std::move(executorInfos),
                                   ExecutionNode::RETURN)
      .setInputValueList(1, 2, 5, 2, 1, 5, 7, 1)
      .setInputSplitType(getSplit())
      .setCall(call)
      .expectOutput({0}, {{1}, {2}, {5}})
      .expectSkipped(0)
      .expectedState(ExecutionState::HASMORE)
      .expectedStats(getCountStats(3))
      .run();
}

TEST_P(ReturnExecutorTest, handle_hard_limit) {
  RegisterInfos registerInfos(RegIdSet{0}, RegIdSet{0}, 1, 1, RegIdFlatSet{},
                              RegIdFlatSetStack{{}});
  ReturnExecutorInfos executorInfos(0 /*input register*/, doCount());
  AqlCall call{};
  call.hardLimit = 5u;
  makeExecutorTestHelper()
      .addConsumer<ReturnExecutor>(std::move(registerInfos),
                                   std::move(executorInfos),
                                   ExecutionNode::RETURN)
      .setInputValueList(1, 2, 5, 2, 1, 5, 7, 1)
      .setInputSplitType(getSplit())
      .setCall(call)
      .expectOutput({0}, {{1}, {2}, {5}, {2}, {1}})
      .expectSkipped(0)
      .expectedState(ExecutionState::DONE)
      .expectedStats(getCountStats(5))
      .run();
}

TEST_P(ReturnExecutorTest, handle_offset) {
  RegisterInfos registerInfos(RegIdSet{0}, RegIdSet{0}, 1, 1, RegIdFlatSet{},
                              RegIdFlatSetStack{{}});
  ReturnExecutorInfos executorInfos(0 /*input register*/, doCount());
  AqlCall call{};
  call.offset = 4;
  makeExecutorTestHelper()
      .addConsumer<ReturnExecutor>(std::move(registerInfos),
                                   std::move(executorInfos),
                                   ExecutionNode::RETURN)
      .setInputValueList(1, 2, 5, 2, 1, 5, 7, 1)
      .setInputSplitType(getSplit())
      .setCall(call)
      .expectOutput({0}, {{1}, {5}, {7}, {1}})
      .expectSkipped(4)
      .expectedState(ExecutionState::DONE)
      .expectedStats(getCountStats(4))
      .run();
}

TEST_P(ReturnExecutorTest, handle_fullcount) {
  RegisterInfos registerInfos(RegIdSet{0}, RegIdSet{0}, 1, 1, RegIdFlatSet{},
                              RegIdFlatSetStack{{}});
  ReturnExecutorInfos executorInfos(0 /*input register*/, doCount());
  AqlCall call{};
  call.hardLimit = 2u;
  call.fullCount = true;
  makeExecutorTestHelper()
      .addConsumer<ReturnExecutor>(std::move(registerInfos),
                                   std::move(executorInfos),
                                   ExecutionNode::RETURN)
      .setInputValueList(1, 2, 5, 2, 1, 5, 7, 1)
      .setInputSplitType(getSplit())
      .setCall(call)
      .expectOutput({0}, {{1}, {2}})
      .expectSkipped(6)
      .expectedState(ExecutionState::DONE)
      .expectedStats(getCountStats(2))
      .run();
}

TEST_P(ReturnExecutorTest, handle_other_inputRegister) {
  RegisterInfos registerInfos(RegIdSet{1}, RegIdSet{0}, 2, 1, RegIdFlatSet{},
                              RegIdFlatSetStack{{}});
  ReturnExecutorInfos executorInfos(1 /*input register*/, doCount());
  AqlCall call{};
  call.hardLimit = 5u;
  makeExecutorTestHelper<2, 1>()
      .addConsumer<ReturnExecutor>(std::move(registerInfos),
                                   std::move(executorInfos),
                                   ExecutionNode::RETURN)
      .setInputValue({{R"("invalid")", 1},
                      {R"("invalid")", 2},
                      {R"("invalid")", 5},
                      {R"("invalid")", 2},
                      {R"("invalid")", 1},
                      {R"("invalid")", 5},
                      {R"("invalid")", 7},
                      {R"("invalid")", 1}})
      .setInputSplitType(getSplit())
      .setCall(call)
      .expectOutput({0}, {{1}, {2}, {5}, {2}, {1}})
      .expectSkipped(0)
      .expectedState(ExecutionState::DONE)
      .expectedStats(getCountStats(5))
      .run();
}
}  // namespace aql
}  // namespace tests
}  // namespace arangodb
