/* jshint globalstrict:false, strict:false, unused: false */
/* global assertEqual, assertFalse, assertNotNull, fail */
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
// / @author Jan Steemann
// / @author Copyright 2013, triAGENS GmbH, Cologne, Germany
// //////////////////////////////////////////////////////////////////////////////

var db = require('@arangodb').db;
var internal = require('internal');
var jsunity = require('jsunity');

function runSetup () {
  'use strict';
  
  // turn off syncing of counters etc.  
  internal.debugSetFailAt("RocksDBSettingsManagerSync"); 

  db._drop('UnitTestsRecovery1');
  db._drop('UnitTestsRecovery2');

  let c = db._createEdgeCollection('UnitTestsRecovery1');
  let c2 = db._create('UnitTestsRecovery2');
  c2.insert({}); // make sure count is initalized

  let docs = [];
  for (let i = 0; i < 100000; i++) {
    docs.push({ _key: "test" + i, _from: "test/1", _to: "test/" + i, value: i });
    if (docs.length === 10000) {
      c.insert(docs);
      docs = [];
    }
  }

  c.ensureIndex({ type: "hash", fields: ["value"] });
  c.ensureIndex({ type: "hash", fields: ["value", "_to"], unique: true });
 
  // should trigger range deletion
  c.truncate();

  c2.insert({}, { waitForSync: true });
  internal.debugTerminate('crashing server');
}

// //////////////////////////////////////////////////////////////////////////////
// / @brief test suite
// //////////////////////////////////////////////////////////////////////////////

function recoverySuite () {
  'use strict';
  jsunity.jsUnity.attachAssertions();

  return {


    testNosyncRangeDeleteTruncateIndexes1: function () {
      let c = db._collection('UnitTestsRecovery1');
      assertEqual(0, c.count());
      assertNotNull(db._collection('UnitTestsRecovery2'));
  
      assertEqual([], c.edges("test/1"));
      let query = "FOR doc IN @@collection FILTER doc.value == @value RETURN doc";
      
      for (let i = 0; i < 100000; i += 1000) {
        assertFalse(c.exists("key" + i));
        assertEqual([], db._query(query, { "@collection": c.name(), value: i }).toArray());
        assertEqual([], c.edges("test/" + i));
      }

      internal.waitForEstimatorSync(); // make sure estimates are consistent
      let indexes = c.indexes(true);
      assertEqual(indexes.length, 4);
      for (let i of indexes) {
        switch (i.type) {
          case 'primary':
          case 'hash':
          case 'edge':
            assertEqual(i.selectivityEstimate, 1, JSON.stringify(indexes));
            break;
          default:
            fail();
        }
      }
    }

  };
}

// //////////////////////////////////////////////////////////////////////////////
// / @brief executes the test suite
// //////////////////////////////////////////////////////////////////////////////

function main (argv) {
  'use strict';
  if (argv[1] === 'setup') {
    runSetup();
    return 0;
  } else {
    jsunity.run(recoverySuite);
    return jsunity.writeDone().status ? 0 : 1;
  }
}
