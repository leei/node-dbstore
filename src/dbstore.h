#ifndef DBSTORE_H
#define DBSTORE_H

#include <node.h>

class DbStore : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);

 private:
  DbStore();
  ~DbStore();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> PlusOne(const v8::Arguments& args);
  double counter_;
};

#endif
