/* jshint globalstrict:false, strict:false, unused : false */
/* global runSetup assertEqual, assertNull */

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
// / @author Copyright 2012, triAGENS GmbH, Cologne, Germany
// //////////////////////////////////////////////////////////////////////////////

var db = require('@arangodb').db;
var internal = require('internal');
var jsunity = require('jsunity');

if (runSetup === true) {
  'use strict';
  global.instanceManager.debugClearFailAt();
  var i;

  db._drop('UnitTestsRecovery1');
  db._drop('UnitTestsRecovery2');
  db._create('UnitTestsRecovery1');
  
  for (i = 0; i < 10000; ++i) {
    db.UnitTestsRecovery1.save({ a: i });
  }

  db._create('UnitTestsRecovery2');
  db._query("FOR doc IN UnitTestsRecovery1 INSERT doc INTO UnitTestsRecovery2");
  
  db._drop('UnitTestsRecovery1');
  db.UnitTestsRecovery2.rename('UnitTestsRecovery1');

  return 0;
}

// //////////////////////////////////////////////////////////////////////////////
// / @brief test suite
// //////////////////////////////////////////////////////////////////////////////

function recoverySuite () {
  'use strict';
  jsunity.jsUnity.attachAssertions();

  return {


    // //////////////////////////////////////////////////////////////////////////////
    // / @brief test whether rename and recreate works
    // //////////////////////////////////////////////////////////////////////////////

    testCollectionRenameWithData: function () {
      var c = db._collection('UnitTestsRecovery1');
      assertEqual(10000, c.count());

      assertNull(db._collection('UnitTestsRecovery2'));
    }

  };
}

// //////////////////////////////////////////////////////////////////////////////
// / @brief executes the test suite
// //////////////////////////////////////////////////////////////////////////////
jsunity.run(recoverySuite);
return jsunity.done();
