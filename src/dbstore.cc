#include <node.h>
#include <node_buffer.h>

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

  tpl->PrototypeTemplate()->Set(String::NewSymbol("_put"),
      FunctionTemplate::New(Put)->GetFunction());
  tpl->PrototypeTemplate()->Set(String::NewSymbol("_get"),
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
  return _db->put(_db, 0, key, data, flags);
}

int 
DbStore::get(DBT *key, DBT *data, u_int32_t flags)
{
  return _db->get(_db, 0, key, data, flags);
}

int 
DbStore::del(DBT *key, u_int32_t flags)
{
  return _db->del(_db, 0, key, flags);
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
  Persistent<Object> data;
  DBT retval;
  int ret;
} WorkBaton;

static void
After(WorkBaton *baton, char const *call, Handle<Value> *argv, int argc)
{
  if (baton->ret)
    argv[0] = node::UVException(0, call, db_strerror(baton->ret));
  else
    argv[0] = Null();

  // surround in a try/catch for safety
  TryCatch try_catch;

  // execute the callback function
  //fprintf(stderr, "%p.%s Calling cb\n", baton->store, call);
  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);

  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  // dispose the Persistent handle so the callback
  // function can be garbage-collected
  baton->callback.Dispose();
}

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
  Handle<Value> argv[1];

  After(baton, "db_open", argv, 1);

  free(baton->fname);

  // clean up any memory we allocated
  delete baton;
  delete req;
}

Handle<Value> DbStore::Open(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

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
  Handle<Value> argv[1];
  After(baton, "db_close", argv, 1);

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

static void 
PutWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;

  DBT key_dbt; memset(&key_dbt, sizeof(key_dbt), 0);
  key_dbt.size = strlen(baton->fname);
  key_dbt.data = baton->fname;
  key_dbt.flags = DB_DBT_USERMEM;

  DBT data_dbt; memset(&data_dbt, sizeof(key_dbt), 0);
  data_dbt.size = node::Buffer::Length(baton->data);
  data_dbt.data = node::Buffer::Data(baton->data);
  data_dbt.flags = DB_DBT_USERMEM;

  baton->ret = store->put(&key_dbt, &data_dbt, 0);
}

static void 
PutAfter(uv_work_t *req) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[1];
  After(baton, "db_put", argv, 1);

  // clean up any memory we allocated
  free(baton->fname);
  delete baton;
  delete req;
}

Handle<Value> DbStore::Put(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  if (! args.Length() > 0 && ! args[0]->IsString()) {
     ThrowException(Exception::TypeError(String::New("First argument must be a string")));
    return scope.Close(Undefined()); 
  }
  String::Utf8Value key(args[0]);

  if (! args.Length() > 1 && ! node::Buffer::HasInstance(args[1])) {
     ThrowException(Exception::TypeError(String::New("Second argument must be a Buffer")));
    return scope.Close(Undefined()); 
  }
  Persistent<Object> data = Persistent<Object>::New(Local<Object>::Cast(args[1]));

  if (! args.Length() > 2 && ! args[2]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be callback function")));
    return scope.Close(Undefined()); 
  }
  Persistent<Function> callback = 
    Persistent<Function>::New(Local<Function>::Cast(args[2]));

// create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton;
  req->data = baton;
  baton->store = obj;

  baton->fname = strdup(*key);
  baton->data = data;
  baton->callback = callback;
  
  uv_queue_work(uv_default_loop(), req, PutWork, PutAfter);

  return args.This();
}

static void 
GetWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;

  DBT key_dbt; memset(&key_dbt, sizeof(key_dbt), 0);
  key_dbt.size = strlen(baton->fname);
  key_dbt.data = baton->fname;
  key_dbt.flags = DB_DBT_USERMEM;

  DBT *retval = &baton->retval;
  memset(retval, sizeof(*retval), 0);
  retval->flags = DB_DBT_MALLOC;

  baton->ret = store->get(&key_dbt, retval, 0);
}

static void 
GetAfter(uv_work_t *req) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[2];
  
  DBT *retval = &baton->retval;
  node::Buffer *buf = node::Buffer::New((char*)retval->data, retval->size);
  argv[1] = buf->handle_;
  After(baton, "db_get", argv, 2);

  // clean up any memory we allocated
  free(baton->fname);
  delete baton;
  delete req;
}

Handle<Value> DbStore::Get(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  if (! args.Length() > 0 && ! args[0]->IsString()) {
     ThrowException(Exception::TypeError(String::New("First argument must be a string")));
    return scope.Close(Undefined()); 
  }
  String::Utf8Value key(args[0]);

  if (! args.Length() > 1 && ! args[1]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be callback function")));
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

  baton->fname = strdup(*key);
  baton->callback = callback;
  
  uv_queue_work(uv_default_loop(), req, GetWork, GetAfter);

  return args.This();
}

static void 
DelWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;

  DBT key_dbt; memset(&key_dbt, sizeof(key_dbt), 0);
  key_dbt.size = strlen(baton->fname);
  key_dbt.data = baton->fname;
  key_dbt.flags = DB_DBT_USERMEM;

  baton->ret = store->del(&key_dbt, 0);
}

Handle<Value> DbStore::Del(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  if (! args.Length() > 0 && ! args[0]->IsString()) {
     ThrowException(Exception::TypeError(String::New("First argument must be a string")));
    return scope.Close(Undefined()); 
  }
  String::Utf8Value key(args[0]);

  if (! args.Length() > 1 && ! args[1]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be callback function")));
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

  baton->fname = strdup(*key);
  baton->callback = callback;
  
  uv_queue_work(uv_default_loop(), req, DelWork, PutAfter); // Yes, use the same.

  return args.This();
}

Handle<Value> DbStore::Sync(const Arguments& args) {
  HandleScope scope;

  //DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  return scope.Close(Number::New(0));
}

