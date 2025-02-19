/* jshint strict: false */

// //////////////////////////////////////////////////////////////////////////////
// / DISCLAIMER
// /
// / Copyright 2014-2024 ArangoDB GmbH, Cologne, Germany
// / Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
// /
// / Licensed under the Business Source License 1.1 (the "License");
// / you may not use this file except in compliance with the License.
// / You may obtain a copy of the License at
// /
// /     https://github.com/arangodb/arangodb/blob/devel/LICENSE
// /
// / Unless required by applicable law or agreed to in writing, software
// / distributed under the License is distributed on an "AS IS" BASIS,
// / WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// / See the License for the specific language governing permissions and
// / limitations under the License.
// /
// / Copyright holder is ArangoDB GmbH, Cologne, Germany
// /
// / @author Achim Brandt
// / @author Dr. Frank Celler
// / @author Copyright 2012-2013, triAGENS GmbH, Cologne, Germany
// //////////////////////////////////////////////////////////////////////////////

const internal = require('internal');
const arangosh = require('@arangodb/arangosh');

const ArangoStatement = require('@arangodb/arango-statement-common').ArangoStatement;
const ArangoQueryCursor = require('@arangodb/arango-query-cursor').ArangoQueryCursor;

// //////////////////////////////////////////////////////////////////////////////
// / @brief return a string representation of the statement
// //////////////////////////////////////////////////////////////////////////////

ArangoStatement.prototype.toString = function () {
  return arangosh.getIdString(this, 'ArangoStatement');
};

// //////////////////////////////////////////////////////////////////////////////
// / @brief prints the help for ArangoStatement
// //////////////////////////////////////////////////////////////////////////////

var helpArangoStatement = arangosh.createHelpHeadline('ArangoStatement help') +
  'Create an AQL query:                                                    ' + '\n' +
  ' > stmt = new ArangoStatement(db, { "query": "FOR..." })                ' + '\n' +
  ' > stmt = db._createStatement({ "query": "FOR..." })                    ' + '\n' +
  'Set query options:                                                      ' + '\n' +
  ' > stmt.setBatchSize(<value>)           set the max. number of results  ' + '\n' +
  '                                        to be transferred per roundtrip ' + '\n' +
  ' > stmt.setCount(<value>)               set count flag (return number of' + '\n' +
  '                                        results in "count" attribute)   ' + '\n' +
  'Get query options:                                                      ' + '\n' +
  ' > stmt.getBatchSize()                  return the max. number of results' + '\n' +
  '                                        to be transferred per roundtrip ' + '\n' +
  ' > stmt.getCount()                      return count flag (return number' + '\n' +
  '                                        of results in "count" attribute)' + '\n' +
  ' > stmt.getQuery()                      return query string             ' + '\n' +
  '                                        results in "count" attribute)   ' + '\n' +
  'Bind parameters to a query:                                             ' + '\n' +
  ' > stmt.bind(<key>, <value>)            bind single variable            ' + '\n' +
  ' > stmt.bind(<values>)                  bind multiple variables         ' + '\n' +
  'Execute query:                                                          ' + '\n' +
  ' > cursor = stmt.execute()              returns a cursor                ' + '\n' +
  'Get all results in an array:                                            ' + '\n' +
  ' > docs = cursor.toArray()                                              ' + '\n' +
  'Or loop over the result set:                                            ' + '\n' +
  ' > while (cursor.hasNext()) { print(cursor.next()) }                    ';

ArangoStatement.prototype._help = function () {
  internal.print(helpArangoStatement);
};

// //////////////////////////////////////////////////////////////////////////////
// / @brief parse a query and return the results
// //////////////////////////////////////////////////////////////////////////////

ArangoStatement.prototype.parse = function () {
  var body = {
    query: this._query
  };

  var requestResult = this._database._connection.POST(
    '/_api/query', body);

  arangosh.checkRequestResult(requestResult);

  var result = {
    bindVars: requestResult.bindVars,
    collections: requestResult.collections,
    ast: requestResult.ast
  };
  return result;
};

// //////////////////////////////////////////////////////////////////////////////
// / @brief explain a query and return the results
// //////////////////////////////////////////////////////////////////////////////

ArangoStatement.prototype.explain = function (options) {
  let opts = this._options || { };
  if (typeof opts === 'object' && typeof options === 'object') {
    Object.keys(options).forEach(function (o) {
      // copy options
      opts[o] = options[o];
    });
  }

  let body = {
    query: this._query,
    bindVars: this._bindVars,
    options: opts
  };

  let requestResult = this._database._connection.POST(
    '/_api/explain', body);

  arangosh.checkRequestResult(requestResult);

  let result = {
    warnings: requestResult.warnings,
    stats: requestResult.stats,
  };
  if (opts && opts.allPlans) {
    result.plans = requestResult.plans;
  } else {
    result.plan = requestResult.plan;
    result.cacheable = requestResult.cacheable;
    if (requestResult.hasOwnProperty("planCacheKey")) {
      result.planCacheKey = requestResult.planCacheKey;
    }
  }
  return result;
};

// //////////////////////////////////////////////////////////////////////////////
// / @brief execute the query
// /
// / Invoking execute() will transfer the query and all bind parameters to the
// / server. It will return a cursor with the query results in case of success.
// / In case of an error, the error will be printed
// //////////////////////////////////////////////////////////////////////////////

ArangoStatement.prototype.execute = function () {
  var body = {
    query: this._query,
    count: this._doCount,
    bindVars: this._bindVars,
    stream: this._stream
  };

  if (this._batchSize) {
    body.batchSize = this._batchSize;
  }

  if (this._options) {
    body.options = this._options;
  }

  if (this._cache !== undefined) {
    body.cache = this._cache;
  }

  if (this._ttl !== undefined) {
    body.ttl = this._ttl;
  }

  var requestResult = this._database._connection.POST(
    '/_api/cursor', body);

  arangosh.checkRequestResult(requestResult);

  let isStream = this._stream;
  if (!isStream && this._options && this._options.stream) {
    isStream = this._options.stream;
  }
  let allowRetry = false;
  if (this._options && this._options.allowRetry) {
    allowRetry = true;
  }
  return new ArangoQueryCursor(this._database, requestResult, isStream, allowRetry);
};

exports.ArangoStatement = ArangoStatement;
