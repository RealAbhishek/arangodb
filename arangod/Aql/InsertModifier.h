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
/// @author Markus Pfeiffer
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Aql/Executor/ModificationExecutor.h"
#include "Aql/Executor/ModificationExecutorAccumulator.h"
#include "Aql/Executor/ModificationExecutorInfos.h"
#include "Futures/Future.h"

namespace arangodb::aql {

struct ModificationExecutorInfos;

class InsertModifierCompletion {
 public:
  explicit InsertModifierCompletion(ModificationExecutorInfos& infos)
      : _infos(infos) {}

  ~InsertModifierCompletion() = default;

  ModifierOperationType accumulate(ModificationExecutorAccumulator& accu,
                                   InputAqlItemRow& row);
  futures::Future<OperationResult> transact(transaction::Methods& trx,
                                            VPackSlice const& data);

 private:
  ModificationExecutorInfos& _infos;
};

}  // namespace arangodb::aql
