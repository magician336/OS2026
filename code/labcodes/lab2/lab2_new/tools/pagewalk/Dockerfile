FROM ubuntu:24.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential gdb gdb-multiarch python3 pkg-config libglib2.0-dev \
    libpixman-1-dev libfdt-dev zlib1g-dev ca-certificates git \
    && rm -rf /var/lib/apt/lists/*

ADD https://codeload.github.com/qemu/qemu/tar.gz/refs/tags/v4.1.1 /tmp/qemu.tar.gz
ADD https://codeload.github.com/qemu/keycodemapdb/tar.gz/6b3d716e2b6472eb7189d3220552280ef3d832ce /tmp/keycodemapdb.tar.gz
RUN echo '30122a74528d4ea0c21acd0799ee017748225cecb860167b8f5547f2ee3447e4  /tmp/qemu.tar.gz' | sha256sum -c - \
    && echo 'c61611a13bb6e7eb6889e63c99ce8d91353eb3f6c58a9ccaffea5f1c511d413a  /tmp/keycodemapdb.tar.gz' | sha256sum -c - \
    && mkdir -p /opt/qemu-src /opt/qemu-build \
    && tar -xzf /tmp/qemu.tar.gz --strip-components=1 -C /opt/qemu-src \
    && tar -xzf /tmp/keycodemapdb.tar.gz --strip-components=1 -C /opt/qemu-src/ui/keycodemapdb \
    && rm /tmp/qemu.tar.gz /tmp/keycodemapdb.tar.gz

WORKDIR /opt/qemu-build
RUN /opt/qemu-src/configure --target-list=riscv64-softmmu \
    --enable-debug --disable-strip --disable-werror --extra-cflags=-O0 \
    --disable-docs --disable-guest-agent --disable-tools --disable-sdl \
    --disable-gtk --disable-vnc --disable-opengl --disable-slirp \
    --disable-capstone --enable-fdt && make -j4

WORKDIR /kernel
