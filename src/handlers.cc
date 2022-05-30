#ifndef IOSTREAM
#define IOSTREAM
#include <iostream>
#endif

#ifndef SYS_SOCKET
#define SYS_SOCKET
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "handlers.h"

namespace Handlers {

Handler::Handler(string name) {
    this->name = name;
}

string Handler::getName() {
    return this->name;
}

URingEvent *ReadFileHandler::handle(URingEvent *event, io_uring *ring, int sock) {
    int size = (int)event->eventInfo.fileInfo->fileSize;
    int block = size / FILE_BLOCK_MAX_SIZE;
    if (size % FILE_BLOCK_MAX_SIZE > 0)
        block++;
    for (int i = 0; i < block; i++) {
        char *buf = (char *)event->eventInfo.fileInfo->iovecs[i].iov_base;
        int len = (int)event->eventInfo.fileInfo->iovecs[i].iov_len;
        while (len--) {
            cout << *buf;
            buf++;
        }
    }
    cout << "\n";
    return NULL;
}

URingEvent *SocketReadHandler::handle(URingEvent *event, io_uring *ring, int sock) {
    cout << "Read from socket: ";
    SockInfo *sinfo = (SockInfo *)event->eventInfo.sockInfo;
    printf("address3: %p\n", event->eventInfo.sockInfo);
    char *buf = (char *)sinfo->msgBuf;
    int len = (int)sinfo->msgSize;
    while (len--) {
        cout << *buf;
        buf++;
    }
    cout << "\nRead Success!\n";

    return NULL;
}

URingEvent *SocketWriteHandler::handle(URingEvent *event, io_uring *ring, int sock) {
    cout << "Sent success!" << endl;
    URingEvent *readEvent = new URingEvent();
    readEvent->eventType = SOCKET_READ_EVENT;
    readEvent->eventInfo.sockInfo = new SockInfo();
    readEvent->eventInfo.sockInfo->msgBuf = event->eventInfo.sockInfo->msgBuf;
    readEvent->eventInfo.sockInfo->msgSize = event->eventInfo.sockInfo->msgSize;
    printf("address1: %p\n", readEvent->eventInfo.sockInfo);

    // Sumbit sqe.
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_recv(sqe, sock, event->eventInfo.sockInfo->msgBuf,
        event->eventInfo.sockInfo->msgSize, 0);
    io_uring_sqe_set_data(sqe, readEvent);
    // memcpy(&sqe->user_data, readEvent, sizeof(readEvent));
    // printf("address2: %p\n", sqe->user_data);
    io_uring_submit(ring);
    cout << "Ready to read..." << endl;

    return readEvent;
}

} // namespace Handlers