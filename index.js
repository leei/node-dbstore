var addon = require("./build/Release/addon.node");

var DbStore = addon.DbStore;

DbStore.DbEnv = addon.DbEnv;

DbStore.prototype.put = function (key, val, opts, cb) {
  if (typeof opts == 'function') {
    cb = opts; opts = {};
  }

  if (opts.json) {
    val = JSON.stringify(val);
  }

  var buf = val;
  if (typeof buf == 'string') {
    buf = new Buffer(val, 'utf8');
  }

  return this._put(key, buf, cb);
};


DbStore.prototype.get = function (key, opts, cb) {
  if (typeof opts == 'function') {
    cb = opts; opts = {};
  } else if (typeof opts == 'string') {
    opts = { encoding: opts };
  }

  return this._get(key, function (err, buf) {
    if (! err) {
      if (opts.encoding) {
	buf = buf.toString(opts.encoding);
      }
      if (opts.json) {
	try {
	  buf = JSON.parse(buf);
	} catch (x) {
	}
      }
    }
    cb(err, buf);
  });
};

module.exports = addon.DbStore;
