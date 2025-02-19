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
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "RestHandler/RestVocbaseBaseHandler.h"

#include <velocypack/Builder.h>

namespace arangodb {
class LocalDocumentId;
namespace transaction {
class Methods;
}

namespace aql {
struct AstNode;
struct Variable;
}  // namespace aql

class RestEdgesHandler : public RestVocbaseBaseHandler {
 public:
  explicit RestEdgesHandler(ArangodServer&, GeneralRequest*, GeneralResponse*);

 public:
  RestStatus execute() override final;
  char const* name() const override final { return "RestEdgesHandler"; }
  RequestLane lane() const override final { return RequestLane::CLIENT_SLOW; }

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief reads all edges in given direction for given vertex
  //////////////////////////////////////////////////////////////////////////////

  bool readEdges();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief reads all edges in given direction for a list of vertices
  //////////////////////////////////////////////////////////////////////////////

  bool readEdgesForMultipleVertices();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief get all edges for a given vertex. Independent from the request
  //////////////////////////////////////////////////////////////////////////////

  bool getEdgesForVertex(std::string const& id, DataSourceId cid,
                         std::string const& collectionName,
                         TRI_edge_direction_e direction,
                         transaction::Methods& trx,
                         std::function<void(LocalDocumentId)> const& cb);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Parse the direction parameter
  //////////////////////////////////////////////////////////////////////////////

  bool parseDirection(TRI_edge_direction_e& direction);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief Validate that the collection exists and is an edge collection
  //////////////////////////////////////////////////////////////////////////////

  bool validateCollection(std::string const& name);
};
}  // namespace arangodb
