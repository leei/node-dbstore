#include <node.h>
#include "dbenv.h"

using namespace v8;

DbEnv::DbEnv() {};
DbEnv::~DbEnv() {};

void DbEnv::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("DbEnv"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("plusOne"),
      FunctionTemplate::New(PlusOne)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("DbEnv"), constructor);
}

Handle<Value> DbEnv::New(const Arguments& args) {
  HandleScope scope;

  DbEnv* obj = new DbEnv();
  obj->counter_ = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> DbEnv::PlusOne(const Arguments& args) {
  HandleScope scope;

  DbEnv* obj = ObjectWrap::Unwrap<DbEnv>(args.This());
  obj->counter_ += 1;

  return scope.Close(Number::New(obj->counter_));
}
