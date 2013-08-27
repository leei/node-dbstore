{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "src/addon.cc", "src/dbstore.cc", "src/dbenv.cc" ],
      "include_dirs": ["./deps/db-6.0.20/build_unix"],
      "link_settings": {
        "libraries": [ "../deps/db-6.0.20/build_unix/libdb.a" ]
      }
    }
  ]
}
