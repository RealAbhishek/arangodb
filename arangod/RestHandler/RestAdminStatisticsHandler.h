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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RestHandler/RestBaseHandler.h"

namespace arangodb {
class RestAdminStatisticsHandler : public RestBaseHandler {
 public:
  RestAdminStatisticsHandler(ArangodServer&, GeneralRequest*, GeneralResponse*);

 public:
  char const* name() const override final {
    return "RestAdminStatisticsHandler";
  }
  /// @brief must be on fast lane so that statistics can always be retrieved,
  /// even from otherwise totally busy servers
  RequestLane lane() const override final { return RequestLane::CLIENT_FAST; }
  RestStatus execute() override final;

 private:
  void getStatistics();
  void getStatisticsDescription();
};
}  // namespace arangodb
