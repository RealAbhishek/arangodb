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

#include "Basics/Result.h"
#include "VocBase/Methods/Version.h"

struct TRI_vocbase_t;

namespace arangodb {
class UpgradeFeature;

namespace velocypack {
class Slice;
}

namespace methods {

struct UpgradeResult {
  UpgradeResult() : type(VersionResult::INVALID), _result() {}
  UpgradeResult(ErrorCode err, VersionResult::StatusCode s)
      : type(s), _result(err) {}
  UpgradeResult(ErrorCode err, std::string_view msg,
                VersionResult::StatusCode s)
      : type(s), _result(err, msg) {}
  VersionResult::StatusCode type;

  // forwarded methods
  bool ok() const { return _result.ok(); }
  bool fail() const { return _result.fail(); }
  ErrorCode errorNumber() const { return _result.errorNumber(); }
  std::string_view errorMessage() const { return _result.errorMessage(); }

  // access methods
  Result const& result() const& { return _result; }
  Result result() && { return std::move(_result); }

 private:
  Result _result;
};

/// Code to create and initialize databases
/// Replaces ugrade-database.js for good
struct Upgrade {
  friend class arangodb::UpgradeFeature;

  enum Flags : uint32_t {
    DATABASE_SYSTEM = (1u << 0),
    DATABASE_ALL = (1u << 1),
    DATABASE_EXCEPT_SYSTEM = (1u << 2),
    // =============
    DATABASE_INIT = (1u << 3),
    DATABASE_UPGRADE = (1u << 4),
    DATABASE_EXISTING = (1u << 5),
    DATABASE_ONLY_ONCE = (1u << 6),  // hint that task should be run on
                                     // database only once. New databases
                                     // should assume this task as executed
    // =============
    CLUSTER_NONE = (1u << 7),
    CLUSTER_LOCAL = (1u << 8),  // agency
    CLUSTER_COORDINATOR_GLOBAL = (1u << 9),
    CLUSTER_DB_SERVER_LOCAL = (1u << 10)
  };

  using TaskFunction = std::function<Result(TRI_vocbase_t&, velocypack::Slice)>;

  struct Task {
    std::string name;
    std::string description;
    uint32_t systemFlag;
    uint32_t clusterFlags;
    uint32_t databaseFlags;
    TaskFunction action;
  };

 public:
  /// @brief initialize _system db in cluster
  /// corresponding to cluster-bootstrap.js
  static UpgradeResult clusterBootstrap(TRI_vocbase_t& system);

  /// @brief create a database
  /// corresponding to local-database.js
  static UpgradeResult createDB(TRI_vocbase_t& vocbase,
                                velocypack::Slice users);

  /// @brief executed on startup for non-coordinators
  /// @param upgrade  Perform an actual upgrade
  /// Corresponds to upgrade-database.js
  static UpgradeResult startup(TRI_vocbase_t& vocbase, bool upgrade,
                               bool ignoreFileErrors);

  /// @brief executed on startup for coordinators
  /// @param upgrade  Perform an actual upgrade
  /// Corresponds to upgrade-database.js
  static UpgradeResult startupCoordinator(TRI_vocbase_t& vocbase);

 private:
  /// @brief register tasks, only run once on startup
  static void registerTasks(UpgradeFeature&);

#ifdef USE_ENTERPRISE
  // Like RegisterTasks, but only dedicated to enterprise based
  // upgrade tasks. Implementation in closed Enterprise Source
  static void registerTasksEE(UpgradeFeature&);
#endif

  static UpgradeResult runTasks(TRI_vocbase_t& vocbase, VersionResult& vinfo,
                                velocypack::Slice params, uint32_t clusterFlag,
                                uint32_t dbFlag);

  /*
  /// @brief system database only
  constexpr int DATABASE_SYSTEM = 1000;
  /// @brief all databases
  constexpr int DATABASE_ALL = 1001;
  /// @brief all databases expect system
  constexpr int DATABASE_EXCEPT_SYSTEM = 1002;
  /// @brief for stand-alone, no cluster
  constexpr int CLUSTER_NONE = 2000;
  /// @brief for cluster local part
  constexpr int CLUSTER_LOCAL = 2001;
  /// @brief for cluster global part (shared collections)
  constexpr int CLUSTER_COORDINATOR_GLOBAL = 2002;
  /// @brief db server global part (DB server local!)
  constexpr int CLUSTER_DB_SERVER_LOCAL = 2003;
  /// @brief for new databases
  constexpr int DATABASE_INIT = 3000;
  /// @brief for existing database, which must be upgraded
  constexpr int DATABASE_UPGRADE = 3001;
  /// @brief for existing database, which are already at the correct version
  constexpr int DATABASE_EXISTING = 3002;*/
};

}  // namespace methods
}  // namespace arangodb
