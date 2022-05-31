#ifndef REACTOR_HANDLERS_HANDLERS_H_
#define REACTOR_HANDLERS_HANDLERS_H_

#include <stdio.h>
#include <string.h>
#include <string>

#include "liburing.h"

#define FILE_HANDLER_NAME "file"
#define SOCKET_READ_HANDLER_NAME "socket_read"
#define SOCKET_WRITE_HANDLER_NAME "socket_write"
#define FILE_BLOCK_MAX_SIZE 1024
#define SERVER_PORT 8000

using namespace std;

enum EventType { READ_FILE_EVENT,
    SOCKET_READ_EVENT,
    SOCKET_WRITE_EVENT,
    EXIT_EVENT };

struct FileInfo {
    off_t fileSize;
    struct iovec iovecs[];
};

struct SockInfo {
    size_t msgSize;
    char *msgBuf;
};

struct URingEvent {
    EventType eventType;
    union EventInfo {
        FileInfo *fileInfo;
        SockInfo *sockInfo;
    } eventInfo;
};

namespace Handlers {

struct Request {
};

class Handler {
public:
    Handler() {
    }

    Handler(string name);

    ~Handler() {
    }

    virtual int handle(URingEvent *event, io_uring *ring, int sock) = 0;

    string getName();

protected:
    string name;
};

class ReadFileHandler : public Handler {
    // Inherit all constructors of Handler class
    using Handler::Handler;

    int handle(URingEvent *event, io_uring *ring, int sock);
};

class SocketReadHandler : public Handler {
    using Handler::Handler;

    int handle(URingEvent *event, io_uring *ring, int sock);
};

class SocketWriteHandler : public Handler {
    using Handler::Handler;

    int handle(URingEvent *event, io_uring *ring, int sock);
};
} // namespace Handlers
#endif