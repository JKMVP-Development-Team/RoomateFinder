# Use official GCC image
FROM gcc:12

# Install dependencies
RUN apt-get -y update \
    && apt-get -y upgrade \
    && apt-get install -y \
    cmake \
    git \
    curl \
    libssl-dev \
    libboost-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libsnappy-dev \
    libz-dev \
    pkg-config \
    build-essential


RUN git clone https://github.com/mongodb/mongo-c-driver.git --branch 1.24.0 /usr/local/src/mongo-c-driver && \
    cd /usr/local/src/mongo-c-driver && \
    mkdir cmake-build && cd cmake-build && \
    cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    cmake --build . --config Release && \
    cmake --build . --target install && \
    ldconfig


RUN git clone https://github.com/mongodb/mongo-cxx-driver.git --branch r3.8.0 /usr/local/src/mongo-cxx-driver && \
    cd /usr/local/src/mongo-cxx-driver && \
    mkdir cmake-build && cd cmake-build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=ON \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_PREFIX_PATH=/usr/local \
          .. && \
    cmake --build . --config Release && \
    cmake --build . --target install && \
    ldconfig

# Set working directory
WORKDIR /app

# Copy only necessary files
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY include/ ./include/

# Configure + build
RUN cmake -Bbuild -S. -DOCKER_BUILD=ON \
    -DCMAKE_PREFIX_PATH="/usr/local;/usr/local/lib;/usr/local/lib/cmake;/usr/local/lib/cmake/libmongocxx-3.8.0;/usr/local/lib/cmake/libbsoncxx-3.8.0" \
    && cmake --build build


# Expose default Crow port (adjust if needed)
EXPOSE 18080

# Run the built binary
CMD ["./build/roommateapp"]
