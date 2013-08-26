#include <node.h>
#include "dbstore.h"

using namespace v8;

DbStore::DbStore() {};
DbStore::~DbStore() {};

void DbStore::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("DbStore"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("plusOne"),
      FunctionTemplate::New(PlusOne)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("DbStore"), constructor);
}

Handle<Value> DbStore::New(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = new DbStore();
  obj->counter_ = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> DbStore::PlusOne(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());
  obj->counter_ += 1;

  return scope.Close(Number::New(obj->counter_));
}
