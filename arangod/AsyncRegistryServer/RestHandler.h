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

#include "AsyncRegistryServer/Feature.h"
#include "RestHandler/RestVocbaseBaseHandler.h"

namespace arangodb::async_registry {

class RestHandler : public arangodb::RestVocbaseBaseHandler {
 public:
  RestHandler(ArangodServer&, GeneralRequest*, GeneralResponse*);

 public:
  char const* name() const override final { return "AsyncRegistryRestHandler"; }
  RequestLane lane() const override final { return RequestLane::CLUSTER_ADMIN; }
  futures::Future<futures::Unit> executeAsync() override;

  Feature& _feature;
};

}  // namespace arangodb::async_registry
