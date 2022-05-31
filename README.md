# Reactor

A simple reactor using io_uring.
Completed `cat` command, `echo` command & `exit` command.
For `cat filepath` command, it reads from a file and outputs it.
For `echo string` command, it sends the message to an echo server and then receives the result from it
For `exit` command, the program exits.

## 1. Install [liburing](https://github.com/axboe/liburing) on Linux Systems

```shell
git clone https://github.com/axboe/liburing
cd liburing
configure
make
make install
```

## 2. Run Echo Server in Background
```shell
git clone https://github.com/frevib/io_uring-echo-server
io_uring_echo_server 8000  >  out.file  2>&1  & 
```

## 3. Compile and Run
```shell
./build_lib.sh
./build.sh
./main
```
