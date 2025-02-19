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
/// @author Kaveh Vahedipour
/// @author Matthew Von-Maszewski
////////////////////////////////////////////////////////////////////////////////

#include "CreateDatabase.h"
#include "ActionBase.h"

#include "ApplicationFeatures/ApplicationServer.h"
#include "Basics/StaticStrings.h"
#include "Basics/VelocyPackHelper.h"
#include "Basics/debugging.h"
#include "Cluster/MaintenanceFeature.h"
#include "Logger/LogMacros.h"
#include "Logger/Logger.h"
#include "Logger/LoggerStream.h"
#include "RestServer/DatabaseFeature.h"
#include "Utils/DatabaseGuard.h"
#include "Utils/OperationOptions.h"
#include "VocBase/Methods/Databases.h"

using namespace arangodb;
using namespace arangodb::application_features;
using namespace arangodb::maintenance;
using namespace arangodb::methods;

CreateDatabase::CreateDatabase(MaintenanceFeature& feature,
                               ActionDescription const& desc)
    : ActionBase(feature, desc) {
  std::stringstream error;

  _labels.emplace(FAST_TRACK);

  if (!desc.has(DATABASE)) {
    error << "database must be specified.";
  }
  TRI_ASSERT(desc.has(DATABASE));

  if (!error.str().empty()) {
    LOG_TOPIC("751ce", ERR, Logger::MAINTENANCE)
        << "CreateDatabase: " << error.str();
    result(TRI_ERROR_INTERNAL, error.str());
    setState(FAILED);
  }
}

CreateDatabase::~CreateDatabase() = default;

bool CreateDatabase::first() {
  VPackSlice users;
  auto database = _description.get(DATABASE);

  LOG_TOPIC("953b1", DEBUG, Logger::MAINTENANCE)
      << "CreateDatabase: creating database " << database;

  TRI_IF_FAILURE("CreateDatabase::first") {
    // simulate DB creation failure
    result(TRI_ERROR_DEBUG);
    _feature.storeDBError(database, result());
    return false;
  }

  Result res;

  try {
    auto& df = _feature.server().getFeature<DatabaseFeature>();
    DatabaseGuard guard(df, StaticStrings::SystemDatabase);

    // Assertion in constructor makes sure that we have DATABASE.
    auto& server = _feature.server();
    res = Databases::create(server, ExecContext::current(),
                            _description.get(DATABASE), users, properties());
    result(res);
    if (res.fail() && res.isNot(TRI_ERROR_ARANGO_DUPLICATE_NAME)) {
      LOG_TOPIC("5fb67", ERR, Logger::MAINTENANCE)
          << "CreateDatabase: failed to create database " << database << ": "
          << res;

      _feature.storeDBError(database, res);
    } else {
      LOG_TOPIC("997c8", DEBUG, Logger::MAINTENANCE)
          << "CreateDatabase: database  " << database << " created";
    }
  } catch (std::exception const& e) {
    std::stringstream error;
    error << "action " << _description << " failed with exception " << e.what();
    LOG_TOPIC("fa073", ERR, Logger::MAINTENANCE)
        << "CreateDatabase: " << error.str();
    result(TRI_ERROR_INTERNAL, error.str());
    _feature.storeDBError(database, res);
  }

  return false;
}
