#ifndef REACTOR_HANDLERS_HANDLERS_H_
#define REACTOR_HANDLERS_HANDLERS_H_

#ifndef STRING
#define STRING
#include <string>
#endif

#ifndef URING
#define URING
#include <liburing.h>
#endif

#ifndef STDIO
#define STDIO
#include <stdio.h>
#endif

#define FILE_HANDLER_NAME "file"
#define SOCKET_HANDLER_NAME "socket"
#define FILE_BLOCK_MAX_SIZE 1024

using namespace std;

enum EventType { READ_FILE_EVENT,
    SOCKET_EVENT };

struct FileInfo {
    off_t fileSize;
    struct iovec iovecs[];
};

struct URingEvent {
    EventType eventType;
    FileInfo *fileInfo;
};

namespace Handlers {

struct Request {
};

class Handler {
public:
    Handler(std::string name);

    virtual ~Handler() {
    }

    virtual int handle(URingEvent *event) = 0;

    std::string getName();

protected:
    std::string name;
};

class ReadFileHandler : public Handler {
    // Inherit all constructors of Handler class
    using Handler::Handler;

    int handle(URingEvent *event);
};

class SocketHandler : public Handler {
    // Inherit all constructors of Handler class
    using Handler::Handler;

    int handle(URingEvent *event);
};
} // namespace Handlers
#endif