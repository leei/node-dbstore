{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "src/addon.cc", "src/dbstore.cc", "src/dbenv.cc" ],
      "include_dirs": [ "../include", "./deps/db-6.0.20/build_unix"],
      "link_settings": {
        "libraries": [ "-L../lib", "-L../deps/db-6.0.20/build_unix", "-ldb-6.0" ]
      }
    }
  ]
}
