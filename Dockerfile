FROM lancern/llvm:7.0.1-stretch
FROM lancern/cxxdev:strech
WORKDIR /caf
COPY ./ ./

WORKDIR /caf-build
ARG CAF_BUILD_TYPE=Release
RUN cmake -G Ninja -DCMAKE_BUILD_TYPE=${CAF_BUILD_TYPE} /caf && \
    cmake --build .
