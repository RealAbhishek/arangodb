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
/// @author Tobias Gödderz
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Aql/AqlCallList.h"

#include <iosfwd>
#include <vector>

namespace arangodb::aql {

// Partial map dep -> call. May be empty.
// IMPORTANT: Are expected to be saved in increasing order (regarding
// dependency)
struct AqlCallSet {
  struct DepCallPair {
    std::size_t dependency{};
    AqlCallList call;
  };
  std::vector<DepCallPair> calls;

  [[nodiscard]] auto empty() const noexcept -> bool;

  [[nodiscard]] auto size() const noexcept -> size_t;
};

auto operator<<(std::ostream& out, AqlCallSet::DepCallPair const& callPair)
    -> std::ostream&;
auto operator<<(std::ostream&, AqlCallSet const&) -> std::ostream&;

}  // namespace arangodb::aql
