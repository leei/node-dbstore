var DbStore = require("..");

var dbstore = new DbStore();

var dbenv = new DbStore.DbEnv();

dbstore.open("foo.db", function (err, val) {
  console.log("opened" + (err ? ": " + err.stack : " ret=" + val));

  if (! err) {
      dbstore.close(function (err, val) {
	  console.log("closed" + (err ? ": " + err.stack : " ret=" + val));
      });
  }
});
