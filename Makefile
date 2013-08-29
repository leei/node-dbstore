DB_BUILD = deps/db-6.0.20/build_unix

all: build_db
	node-gyp build

clean:
	$(MAKE) -C $(DB_BUILD) clean
	node-gyp clean

config:
	mkdir -p $(DB_BUILD)
	cd $(DB_BUILD) && ../dist/configure --disable-shared #--enable-debug
	node-gyp configure

build_db:
	$(MAKE) -C $(DB_BUILD) libdb-6.0.a


