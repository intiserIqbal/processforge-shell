FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    gdb \
    valgrind \
    libreadline-dev

WORKDIR /processforge

COPY . .

RUN make

CMD ["./build/processforge"]