/* jshint globalstrict:false, strict:false, maxlen : 4000 */
/* global assertEqual, assertTrue */

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
// / @author Wilfried Goesgens
// / @author Copyright 2012, triAGENS GmbH, Cologne, Germany
// //////////////////////////////////////////////////////////////////////////////

var internal = require('internal');
var jsunity = require('jsunity');

// //////////////////////////////////////////////////////////////////////////////
// / @brief test suite
// //////////////////////////////////////////////////////////////////////////////

function dumpTestSuite () {
  'use strict';
  var db = internal.db;

  return {

// //////////////////////////////////////////////////////////////////////////////
// / @brief set up
// //////////////////////////////////////////////////////////////////////////////

    setUp: function () {
    },

// //////////////////////////////////////////////////////////////////////////////
// / @brief tear down
// //////////////////////////////////////////////////////////////////////////////

    tearDown: function () {
    },

// //////////////////////////////////////////////////////////////////////////////
// / @brief test the empty collection
// //////////////////////////////////////////////////////////////////////////////

    testKnowsGraph: function () {
      var g = db._collection('_graphs');
      var e = db._collection('knows');
      var v = db._collection('persons');

      assertTrue(!!g.exists('knows_graph'));

      assertEqual(3, e.type()); // edge
      assertEqual(2, e.indexes().length);
      assertEqual('edge', e.indexes()[1].type);
      assertEqual(5, e.count());

      assertEqual(2, v.type()); // document
      assertEqual(1, v.indexes().length); // just primary index
      assertEqual('primary', v.indexes()[0].type);
      assertEqual(5, v.count());
    }
  };
}

// //////////////////////////////////////////////////////////////////////////////
// / @brief executes the test suite
// //////////////////////////////////////////////////////////////////////////////

jsunity.run(dumpTestSuite);

return jsunity.done();

