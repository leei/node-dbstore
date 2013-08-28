var DbStore = require("..");

var dbstore = new DbStore();

var dbenv = new DbStore.DbEnv();

var async = require('async');
var assert = require('assert');

dbstore.open("foo.db", function (err, val) {
  console.log("opened" + (err ? ": " + err.stack : " ret=" + val));

  var keys = [];
  for (var i = 0; i < 5000; ++i) {
    keys[i] = (Math.random() * 1e6).toFixed(0);
  }
  async.forEach(keys, function (key, next) {
    //console.log("put " + key);
    dbstore.put(key, key, function (err) {
      assert(! err);
      dbstore.get(key, function (err, str) {
	assert(! err);
	//console.log("get " + key + " => " + str);
	assert(str == key);
	dbstore.del(key, next);
      });
    });
  }, function (err) {
    dbstore.close(function (err, val) {
      console.log("closed" + (err ? ": " + err.stack : " ret=" + val));
    });
  });
});
