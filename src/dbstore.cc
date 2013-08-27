#include <node.h>
#include "dbstore.h"

using namespace v8;

DbStore::DbStore() : _db(0), _env(0), _txn(0) {};
DbStore::~DbStore() {
  fprintf(stderr, "~DbStore %p\n", this);
  close();
};

void DbStore::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("DbStore"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("open"),
      FunctionTemplate::New(Open)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("close"),
      FunctionTemplate::New(Close)->GetFunction());

  tpl->PrototypeTemplate()->Set(String::NewSymbol("put"),
      FunctionTemplate::New(Put)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("get"),
      FunctionTemplate::New(Get)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("del"),
      FunctionTemplate::New(Del)->GetFunction());

  tpl->PrototypeTemplate()->Set(String::NewSymbol("sync"),
      FunctionTemplate::New(Sync)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("DbStore"), constructor);
}


int
DbStore::open(char const *fname, char const *db,
	      DBTYPE type, u_int32_t flags, int mode)
{
  int ret = db_create(&_db, NULL, 0);
  if (ret) return ret;
  
  fprintf(stderr, "%p: open %p\n", this, _db);
  return _db->open(_db, NULL, fname, db, type, flags, mode);
}

int 
DbStore::close()
{
  int ret = 0;
  if (_db && _db->pgsize) {
    fprintf(stderr, "%p: close %p\n", this, _db);
    ret = _db->close(_db, 0);
    _db = NULL;
  }
  return ret;
}

int 
DbStore::put(DBT *key, DBT *data, u_int32_t flags)
{
  return 0;
}

int 
DbStore::get(DBT *key, DBT *data, u_int32_t flags)
{
  return 0;
}

int 
DbStore::del(DBT *key, u_int32_t flags)
{
  return 0;
}

int 
DbStore::sync(u_int32_t flags)
{
  return 0;
}

Handle<Value> DbStore::New(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = new DbStore();
  obj->Wrap(args.This());

  return args.This();
}

typedef struct {
  DbStore *store;
  Persistent<Function> callback;
  char *fname;
  int flags;
  DBT retval;
  int ret;
} WorkBaton;

void 
OpenWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;
  baton->ret = store->open(baton->fname, NULL, DB_BTREE, DB_CREATE|DB_THREAD, 0);
}

void 
OpenAfter(uv_work_t *req) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[] = {
    Null(),
    Number::New(baton->ret)
  };

  // surround in a try/catch for safety
  TryCatch try_catch;
  // execute the callback function
  baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  // dispose the Persistent handle so the callback
  // function can be garbage-collected
  baton->callback.Dispose();
  free(baton->fname);

  // clean up any memory we allocated
  delete baton;
  delete req;
}

Handle<Value> DbStore::Open(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  int len = args.Length();

  if (! args[0]->IsString()) {
    ThrowException(Exception::TypeError(String::New("First argument must be String")));
    return scope.Close(Undefined());
  }

  if (! args[1]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Second argument must be callback function")));
    return scope.Close(Undefined()); 
  }
  Persistent<Function> callback = 
    Persistent<Function>::New(Local<Function>::Cast(args[1]));

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton;
  req->data = baton;
  baton->store = obj;

  String::Utf8Value fname(args[0]);
  baton->fname = strdup(*fname);
  baton->callback = callback;
  
  uv_queue_work(uv_default_loop(), req, OpenWork, OpenAfter);

  return args.This();
}

static void 
CloseWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;
  baton->ret = store->close();
}

static void 
CloseAfter(uv_work_t *req) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[] = {
    Null(),
    Number::New(baton->ret)
  };

  // surround in a try/catch for safety
  TryCatch try_catch;
  // execute the callback function
  baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  // dispose the Persistent handle so the callback
  // function can be garbage-collected
  baton->callback.Dispose();

  // clean up any memory we allocated
  delete baton;
  delete req;
}

Handle<Value> DbStore::Close(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  if (! args[0]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be callback function")));
    return scope.Close(Undefined()); 
  }
  Persistent<Function> callback = 
    Persistent<Function>::New(Local<Function>::Cast(args[0]));

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton;
  req->data = baton;
  baton->store = obj;

  baton->callback = callback;
  
  uv_queue_work(uv_default_loop(), req, CloseWork, CloseAfter);
  
  return args.This();
}

Handle<Value> DbStore::Put(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  return scope.Close(Number::New(0));
}

Handle<Value> DbStore::Get(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  return scope.Close(Number::New(0));
}

Handle<Value> DbStore::Del(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  return scope.Close(Number::New(0));
}

Handle<Value> DbStore::Sync(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  return scope.Close(Number::New(0));
}

