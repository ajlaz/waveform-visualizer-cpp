# Build stage
FROM debian:12.10-slim AS builder

RUN apt-get update && apt-get install -y \
    libsdl2-dev \
    libportaudio2 \
    portaudio19-dev \
    libfftw3-dev \
    libglew-dev \
    libgl1-mesa-dev \
    cmake \
    build-essential \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel $(nproc)

# Runtime stage
FROM debian:12.10-slim

RUN apt-get update && apt-get install -y \
    libsdl2-2.0-0 \
    libportaudio2 \
    libfftw3-double3 \
    libfftw3-single3 \
    libglew2.2 \
    libgl1-mesa-glx \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/visualizer .

ENTRYPOINT ["./visualizer"]
CMD ["--server"]