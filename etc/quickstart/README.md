# mongo-c-driver-async Quick Start

The following steps use vcpkg to install mongo-c-driver-async, build a test application, and connect to a local MongoDB server.

Steps were run on Debian 12.

Install vcpkg (if not already installed):
```bash
cd $HOME
git clone https://github.com/microsoft/vcpkg.git
VCPKG_ROOT=$HOME/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
export PATH=$VCPKG_ROOT:$PATH
```

Install amongoc:
```bash
cd $HOME
AMONGOC_INSTALL="$HOME/mongo-c-driver-async-0.1.0"
git clone https://github.com/mongodb-labs/mongo-c-driver-async/
cd mongo-c-driver-async
cmake -B_build 
cmake --build _build --parallel
cmake --install _build --prefix "$AMONGOC_INSTALL"
```

Build this test application:
```bash
# Run from this directory.
cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
      -DCMAKE_PREFIX_PATH=$AMONGOC_INSTALL \
      -Bcmake-build
cmake --build cmake-build --target hello-amongoc
```

Start a local MongoDB server:
```bash
cd $HOME
# Get download links from: https://www.mongodb.com/try/download/community
wget https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-debian12-8.0.4.tgz
tar -xf mongodb-linux-x86_64-debian12-8.0.4.tgz
rm mongodb-linux-x86_64-debian12-8.0.4.tgz
# Start server
mkdir mongodb-data
./mongodb-linux-x86_64-debian12-8.0.4/bin/mongod --dbpath ./mongodb-data
```

Run test application
```bash
./cmake-build/hello-amongoc
```

Expect to see `Successfully connected!`.
