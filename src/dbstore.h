#ifndef DBSTORE_H
#define DBSTORE_H

#include <node.h>

#include <db.h>

class DbStore : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);

  int open(char const *fname, char const *db, DBTYPE type, u_int32_t flags, int mode);
  int close();

  int put(DBT *key, DBT *data, u_int32_t flags);
  int get(DBT *key, DBT *data, u_int32_t flags);
  int del(DBT *key, u_int32_t flags);

  int sync(u_int32_t flags);

 private:
  DbStore();
  ~DbStore();

  DB *_db;
  DB_ENV *_env;
  DB_TXN *_txn;

  static v8::Handle<v8::Value> New(const v8::Arguments& args);

  static v8::Handle<v8::Value> Open(const v8::Arguments& args);
  static v8::Handle<v8::Value> Close(const v8::Arguments& args);

  static v8::Handle<v8::Value> Get(const v8::Arguments& args);
  static v8::Handle<v8::Value> Put(const v8::Arguments& args);
  static v8::Handle<v8::Value> Del(const v8::Arguments& args);

  static v8::Handle<v8::Value> Sync(const v8::Arguments& args);
};

#endif
