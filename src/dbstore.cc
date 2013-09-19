#include <node.h>
#include <node_buffer.h>

#include "dbstore.h"

#include <cstdlib>

using namespace v8;

DbStore::DbStore() : _db(0), _env(0), _txn(0) {};
DbStore::~DbStore() {
  //fprintf(stderr, "~DbStore %p\n", this);
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

  //fprintf(stderr, "%p: open %p\n", this, _db);
  return _db->open(_db, NULL, fname, db, type, flags, mode);
}

int
DbStore::close()
{
  int ret = 0;
  if (_db && _db->pgsize) {
    //fprintf(stderr, "%p: close %p\n", this, _db);
    ret = _db->close(_db, 0);
    _db = NULL;
  }
  return ret;
}

static void
dbt_set(DBT *dbt, void *data, u_int32_t size, u_int32_t flags = DB_DBT_USERMEM)
{
  memset(dbt, 0, sizeof(*dbt));
  dbt->data = data;
  dbt->size = size;
  dbt->flags = flags;
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

struct WorkBaton {
  uv_work_t *req;
  DbStore *store;

  char *str_arg;
  Persistent<Value> data;
  Persistent<Function> callback;

  char const *call;
  DBT inbuf;
  DBT retbuf;
  int ret;

  WorkBaton(uv_work_t *_r, DbStore *_s);
  ~WorkBaton();
};


WorkBaton::WorkBaton(uv_work_t *_r, DbStore *_s) : req(_r), store(_s), str_arg(0) {
  //fprintf(stderr, "new WorkBaton %p:%p\n", this, req);
}
WorkBaton::~WorkBaton() {
  //fprintf(stderr, "~WorkBaton %p:%p\n", this, req);
  delete req;

  if (str_arg) free(str_arg);
  data.Dispose();
  callback.Dispose();
  // Ignore retbuf since it will be freed by Buffer
}

static void
After(WorkBaton *baton, Handle<Value> *argv, int argc)
{
  if (baton->ret) {
    //fprintf(stderr, "%s %s error %d\n", baton->call, baton->str_arg, baton->ret);
    argv[0] = node::UVException(0, baton->call, db_strerror(baton->ret));
  } else {
    argv[0] = Local<Value>::New(Null());
  }

  // surround in a try/catch for safety
  TryCatch try_catch;

  // execute the callback function
  //fprintf(stderr, "%p.%s Calling cb\n", baton->store, call);
  baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);

  if (try_catch.HasCaught())
    node::FatalException(try_catch);

  delete baton;
}

void
OpenWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;
  baton->call = "open";
  baton->ret = store->open(baton->str_arg, NULL, DB_BTREE, DB_CREATE|DB_THREAD, 0);
}

void
OpenAfter(uv_work_t *req, int status) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[1];

  After(baton, argv, 1);
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

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton(req, obj);
  req->data = baton;

  String::Utf8Value fname(args[0]);
  baton->str_arg = strdup(*fname);
  baton->callback = Persistent<Function>::New(Handle<Function>::Cast(args[1]));

  uv_queue_work(uv_default_loop(), req, OpenWork, (uv_after_work_cb)OpenAfter);

  return args.This();
}

static void
CloseWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;
  baton->call = "close";
  baton->ret = store->close();
}

static void
CloseAfter(uv_work_t *req, int status) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[1];
  After(baton, argv, 1);
}

Handle<Value> DbStore::Close(const Arguments& args) {
  HandleScope scope;

  DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  if (! args[0]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be callback function")));
    return scope.Close(Undefined());
  }

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton(req, obj);
  req->data = baton;

  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[0]));

  uv_queue_work(uv_default_loop(), req, CloseWork, (uv_after_work_cb)CloseAfter);

  return args.This();
}

static void
PutWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;
  //fprintf(stderr, "DbStore::PutWork baton %p:%p\n", baton, req);

  DbStore *store = baton->store;

  DBT key_dbt;
  dbt_set(&key_dbt, baton->str_arg, strlen(baton->str_arg));
  DBT &data_dbt = baton->inbuf;

  baton->call = "put";
  //fprintf(stderr, "put %s %p[%d]\n", baton->str_arg, data_dbt.data, data_dbt.size);
  baton->ret = store->put(&key_dbt, &baton->inbuf, 0);
}

static void
PutAfter(uv_work_t *req, int status) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;
  //fprintf(stderr, "DbStore::PutAfter baton %p:%p\n", baton, req);

  // create an arguments array for the callback
  Handle<Value> argv[1];
  After(baton, argv, 1);
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
  Handle<Object> buf = args[1]->ToObject();

  if (! args.Length() > 2 && ! args[2]->IsFunction()) {
    ThrowException(Exception::TypeError(String::New("Argument must be callback function")));
    return scope.Close(Undefined());
  }
  Handle<Function> cb = Handle<Function>::Cast(args[2]);

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton(req, obj);
  //fprintf(stderr, "DbStore::Put %p baton %p\n", req, baton);
  req->data = baton;

  baton->str_arg = strdup(*key);

  dbt_set(&baton->inbuf,
          node::Buffer::Data(buf),
          node::Buffer::Length(buf));

  baton->data = Persistent<Value>::New(buf); // Ensure not GCed until complete
  baton->callback = Persistent<Function>::New(cb);

  uv_queue_work(uv_default_loop(), req, PutWork, (uv_after_work_cb)PutAfter);

  return args.This();
}

static void
GetWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;

  DBT key_dbt;
  dbt_set(&key_dbt, baton->str_arg, strlen(baton->str_arg));

  DBT *retbuf = &baton->retbuf;
  dbt_set(retbuf, 0, 0, DB_DBT_MALLOC);

  baton->call = "get";
  baton->ret = store->get(&key_dbt, retbuf, 0);
  //fprintf(stderr, "get %s => %p[%d]\n", baton->str_arg, key_dbt.data, key_dbt.size);
}

static void
free_buf(char *data, void *hint)
{
  //fprintf(stderr, "Free %p\n", data);
  free(data);
}

static void
GetAfter(uv_work_t *req, int status) {
  HandleScope scope;

  // fetch our data structure
  WorkBaton *baton = (WorkBaton *)req->data;

  // create an arguments array for the callback
  Handle<Value> argv[2];

  DBT *retbuf = &baton->retbuf;
  node::Buffer *buf = node::Buffer::New((char*)retbuf->data, retbuf->size,
                                        free_buf, NULL);
  argv[1] = buf->handle_;
  After(baton, argv, 2);
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

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton(req, obj);
  req->data = baton;

  baton->str_arg = strdup(*key);
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));

  uv_queue_work(uv_default_loop(), req, GetWork, (uv_after_work_cb)GetAfter);

  return args.This();
}

static void
DelWork(uv_work_t *req) {
  WorkBaton *baton = (WorkBaton *) req->data;

  DbStore *store = baton->store;

  DBT key_dbt;
  dbt_set(&key_dbt, baton->str_arg, strlen(baton->str_arg));

  //fprintf(stderr, "%p/%p: del %s\n", baton, req, baton->str_arg);
  baton->call = "del";
  baton->ret = store->del(&key_dbt, 0);
  //fprintf(stderr, "%p/%p: del %s => %d\n", baton, req, baton->str_arg, baton->ret);
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

  // create an async work token
  uv_work_t *req = new uv_work_t;

  // assign our data structure that will be passed around
  WorkBaton *baton = new WorkBaton(req, obj);
  req->data = baton;

  baton->str_arg = strdup(*key);
  baton->callback = Persistent<Function>::New(Local<Function>::Cast(args[1]));

  uv_queue_work(uv_default_loop(), req, DelWork, (uv_after_work_cb)PutAfter); // Yes, use the same.

  return args.This();
}

Handle<Value> DbStore::Sync(const Arguments& args) {
  HandleScope scope;

  //DbStore* obj = ObjectWrap::Unwrap<DbStore>(args.This());

  return scope.Close(Number::New(0));
}

