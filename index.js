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

  var dbstore = this;
  function put(buf) {
    return dbstore._put(key, buf, cb);
  }

  if (! opts.zlib) {
    return put(buf);
  }
  
  var zlib = require('zlib');
  zlib.deflateRaw(buf, function (err, new_buf) {
    if (err) { return cb(err); }
    put(new_buf);
  });
};


DbStore.prototype.get = function (key, opts, cb) {
  if (typeof opts == 'function') {
    cb = opts; opts = {};
  } else if (typeof opts == 'string') {
    opts = { encoding: opts };
  }

  return this._get(key, function (err, buf) {
    if (err) { return cb(err, buf); }

    function decode(buf) {
      if (opts.encoding || opts.json) {
	buf = buf.toString(opts.encoding || 'utf8');
      }
      if (opts.json) {
	try {
	  buf = JSON.parse(buf);
	} catch (x) {
	  err = x;
	}
      }
      cb(err, buf);
    }

    if (! opts.zlib) {
      return decode(buf);
    }
      
    var zlib = require('zlib');
    zlib.inflateRaw(buf, function (err, new_buf) {
      if (err) { return cb(err, buf); }
      decode(new_buf);
    });
  });
};

module.exports = addon.DbStore;
