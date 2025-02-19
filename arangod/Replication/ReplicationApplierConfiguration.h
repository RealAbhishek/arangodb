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
/// @author Jan Steemann
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <set>
#include <string>

#include "RestServer/arangod.h"

namespace arangodb {
namespace application_features {
class ApplicationServer;
}
namespace velocypack {
class Builder;
class Slice;
}  // namespace velocypack

class StorageEngine;

/// @brief struct containing a replication apply configuration
class ReplicationApplierConfiguration {
 public:
  enum class RestrictType { None, Include, Exclude };

  ArangodServer& _server;
  ReplicationFeature* _replicationFeature;

  std::string _endpoint;
  std::string _database;
  std::string _username;
  std::string _password;
  std::string _jwt;
  double _requestTimeout;
  double _connectTimeout;
  uint64_t _ignoreErrors;
  uint64_t _maxConnectRetries;
  uint64_t _lockTimeoutRetries;
  uint64_t _chunkSize;
  uint64_t _connectionRetryWaitTime;
  uint64_t _idleMinWaitTime;
  uint64_t _idleMaxWaitTime;
  uint64_t _initialSyncMaxWaitTime;
  uint64_t _autoResyncRetries;
  uint64_t _maxPacketSize;
  uint32_t _sslProtocol;
  bool _skipCreateDrop;  /// shards/indexes/views are created by schmutz++
  bool _autoStart;       /// start applier after server start
  bool _adaptivePolling;
  bool _autoResync;  /// resync completely if we miss updates
  bool _includeSystem;
  bool _includeFoxxQueues;   /// sync the _jobs and _queues collection
  bool _requireFromPresent;  /// while tailing WAL: leader must have the
                             /// client's requested tick
  bool _incremental;         /// use incremental sync if we got local data
  bool _verbose;
  RestrictType _restrictType;
  std::set<std::string> _restrictCollections;
  std::string _clientInfoString;

 public:
  explicit ReplicationApplierConfiguration(ArangodServer&);
  ~ReplicationApplierConfiguration() = default;

  ReplicationApplierConfiguration(ReplicationApplierConfiguration const&) =
      default;
  ReplicationApplierConfiguration& operator=(
      ReplicationApplierConfiguration const&);

  ReplicationApplierConfiguration(ReplicationApplierConfiguration&&) = default;

  /// @brief reset the configuration to defaults
  void reset();

  /// @brief validate the configuration. will throw if the config is invalid
  void validate() const;

  /// @brief get a VelocyPack representation
  /// expects builder to be in an open Object state
  void toVelocyPack(arangodb::velocypack::Builder&, bool includePassword,
                    bool includeJwt) const;

  void setClientInfo(std::string&& clientInfo) {
    _clientInfoString = std::move(clientInfo);
  }
  void setClientInfo(std::string const& clientInfo) {
    _clientInfoString = clientInfo;
  }

  /// @brief create a configuration object from velocypack
  static ReplicationApplierConfiguration fromVelocyPack(
      ArangodServer&, arangodb::velocypack::Slice slice,
      std::string const& databaseName);

  /// @brief create a configuration object from velocypack, merging it with an
  /// existing one
  static ReplicationApplierConfiguration fromVelocyPack(
      ReplicationApplierConfiguration const& existing,
      arangodb::velocypack::Slice slice, std::string const& databaseName);

  static RestrictType restrictTypeFromString(std::string const& value);
  static std::string restrictTypeToString(RestrictType type);
};

}  // namespace arangodb
