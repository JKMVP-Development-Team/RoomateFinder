# Use official GCC image
FROM gcc:12

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    curl \
    build-essential

# Set working directory
WORKDIR /app

# Copy only necessary files
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY include/ ./include/

# Configure + build
RUN cmake -Bbuild -S. \
 && cmake --build build

# Expose default Crow port (adjust if needed)
EXPOSE 18080

# Run the built binary
CMD ["./build/roommateapp"]
