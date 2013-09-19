var DbStore = require("..");

var dbstore = new DbStore();

var dbenv = new DbStore.DbEnv();

var async = require('async');
var assert = require('assert');

dbstore.open("foo.db", function (err, val) {
  console.log("opened" + (err ? ": " + err.stack : " ret=" + val));
  assert.ifError(err);
    
  function test_put_get(done) {
    console.log("-- test_put_get");
    var keys = [];
    for (var i = 0; i < 5000; ++i) {
      var key = (Math.random() * 1e6).toFixed(0); 
      keys[key] = (Math.random() * 1e6).toFixed(0);
    }

    var dels = {};
    async.forEach(Object.keys(keys), function (key, next) {
      console.log("put " + key);
      dbstore.put(key, keys[key], function (err) {
	assert.ifError(err);
	dbstore.get(key, function (err, str) {
	  assert.ifError(err);
	  console.log("get " + key + " => " + str);
	  assert(str == keys[key]);

	  console.log("del " + key);
	  dbstore.del(key, function (err) {
	    if (err) { console.error("del key[" + key + "] " + err.stack); }
	    assert.ifError(err);

	    dbstore.get(key, function(err, str) {
	      assert(err);
	      next();
	    });
	  });
	});
      });
    }, done);
  }

  function test_json(done) {
    console.log("-- test_json");
    var opts = { json: true };
    var put_data = { test: "json1", n: 1 };
    dbstore.put("json1", put_data, opts, function (err) {
      assert.ifError(err);
      dbstore.get("json1", opts, function (err, data) {
	assert.ifError(err);
	assert(typeof data == 'object');
	assert(data.test == put_data.test);
	assert(data.n == put_data.n);
	dbstore.del("json1", done);
      });
    });
  }

  async.series([test_put_get, test_json], function (err) {
    assert.ifError(err);
    dbstore.close(function (err, val) {
      console.log("closed" + (err ? ": " + err.stack : " ret=" + val));
      assert.ifError(err);
    });
  });
});
