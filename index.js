var addon = require("./build/Release/addon.node");

var DbStore = addon.DbStore;

DbStore.DbEnv = addon.DbEnv;

module.exports = addon.DbStore;
