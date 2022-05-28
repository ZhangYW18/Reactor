#ifndef IOSTREAM
#define IOSTREAM

#include <iostream>

#endif

#include "handlers.h"

namespace Handlers {

Handler::Handler(std::string name) {
    this->name = name;
}

std::string Handler::getName() {
    return this->name;
}

int ReadFileHandler::handle(URingEvent *event) {
    int size = (int)event->fileInfo->fileSize;
    int block = size / FILE_BLOCK_MAX_SIZE;
    if (size % FILE_BLOCK_MAX_SIZE > 0)
        block++;
    for (int i = 0; i < block; i++) {
        char *buf = (char *)event->fileInfo->iovecs[i].iov_base;
        int len = (int)event->fileInfo->iovecs[i].iov_len;
        while (len--) {
            cout << *buf;
            buf++;
        }
    }
    return 0;
}

int SocketHandler::handle(URingEvent *event) {
    std::cout << "SocketHandler Handle" << std::endl;
    return 0;
}

} // namespace Handlers