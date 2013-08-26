//#define BUILDING_NODE_EXTENSION
#include <node.h>

#include "dbstore.h"
#include "dbenv.h"

using namespace v8;

void InitAll(Handle<Object> exports) {
  DbStore::Init(exports);
  DbEnv::Init(exports);
}

NODE_MODULE(addon, InitAll)
