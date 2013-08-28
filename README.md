A very simple set of bindings to Berkeley DB 6.x

## node-dbstore

My goal here was to create a simple, maintained interface to Berkeley DB for caching data for long-running, memory-hog node processes (I have a few of these...)

It's essentially the simplest layer possible for open/close and put/get/del. I'm trying to do the least possible in C++ and provide more advanced functionality with the node layer on top of that.

Lee Iverson

# Installation 

  npm install dbstore



