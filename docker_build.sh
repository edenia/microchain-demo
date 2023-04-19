#!/usr/bin/env sh
set -e

# Set the Docker image name and tag
IMAGE_NAME="contract-builder"
TAG="latest"

# Check if the Docker image already exists
if docker images | grep -q "$IMAGE_NAME"; then
  echo "Docker image $IMAGE_NAME already exists."
else
  echo "Building Docker image $IMAGE_NAME:$TAG ..."
  # Build the Docker image
  docker build -t "$IMAGE_NAME:$TAG" -f docker/contract-builder.Dockerfile .
fi

docker run \
    --rm \
    --mount type=bind,src="$PWD",target=/workspace \
    --workdir /workspace \
    contract-builder:latest bash -c "
        set -e
        git config --global --add safe.directory '*'
        git submodule update --init --recursive
        mkdir -p build-docker
        cd build-docker
        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache ..
        make -j
        ctest -j10 -V
"
