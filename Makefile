DB_BUILD = deps/db-6.0.20/build_unix

all: build_db
	node-gyp build

clean:
	$(MAKE) -C $(DB_BUILD) clean
	node-gyp clean

config:
	mkdir -p $(DB_BUILD)
	TOP=`pwd` && cd $(DB_BUILD) && ../dist/configure --enable-debug --prefix=$$TOP #--disable-shared 
	node-gyp configure

build_db:
	$(MAKE) -C $(DB_BUILD) install


