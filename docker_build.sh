#!/usr/bin/env sh
set -e
docker build -t contract-builder:latest -f docker/contract-builder.Dockerfile .
docker run \
    --rm \
    --mount type=bind,src="$PWD",target=/workspace \
    --workdir /workspace \
    contract-builder:latest bash -c "
        set -e
        git config --global --add safe.directory '*'
        git submodule update --init --recursive
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache ..
        make -j
        ctest -j10 -V 
"
