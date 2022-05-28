#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <unordered_map>

#include <liburing.h>

#include "handlers.h"

#define QUEUE_DEPTH 5

using namespace std;

class Reactor {
public:
    Reactor() {
        /* Initialize io_uring */
        io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    }

    ~Reactor() {
        cout << "Exit reactor..." << endl;
        io_uring_queue_exit(&ring);
    }

    struct io_uring *getIOring() {
        return &(this->ring);
    }

    void registerHandler(Handlers::Handler *handler) {
        this->handlers[handler->getName()] = handler;
    }

    void run() {
        // struct io_uring_cqe *cqe;
        cout << "Reactor is running..." << endl;

        while (true) {
            io_uring_cqe *cqe;
            int ret = io_uring_wait_cqe(&ring, &cqe);
            if (ret < 0) {
                perror("io_uring_wait_cqe error");
                cout << ret << endl;
                continue;
            }
            if (cqe->res < 0) {
                perror("io_uring_wait_cqe cqe res error");
                continue;
            }
            URingEvent *event;
            event = (URingEvent *)io_uring_cqe_get_data(cqe);
            switch (event->eventType) {
            case READ_FILE_EVENT:
                // cout << "File Size cqe: " << event->fileInfo->fileSize << endl;
                handlers[FILE_HANDLER_NAME]->handle(event);
                break;
            case SOCKET_EVENT:
                handlers[SOCKET_HANDLER_NAME]->handle(event);
                break;
            default:
                fprintf(stderr, "Error: Unknown CQE event.\n");
                break;
            }
            io_uring_cqe_seen(&ring, cqe);
        }
    }

private:
    std::unordered_map<std::string, Handlers::Handler *> handlers;
    struct io_uring ring;
};

void *listenAndSubmitEvent(void *ringarg) {
    struct io_uring *ring = (struct io_uring *)ringarg;

    while (true) {
        string input;
        getline(cin, input);
        int space = input.find(" ");
        if (space < 0) {
            fprintf(stderr, "Input is invalid.\n");
            continue;
        }
        string command = input.substr(0, space);
        if (command == "cat") {
            // Get file path
            string tmp = input.substr(space + 1, input.length());
            char filePath[tmp.length() + 1];
            strcpy(filePath, tmp.c_str());
            // cout << "file: " << tmp << endl;
            // Open file
            int fd = open(filePath, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "Open file failed.\n");
                continue;
            }
            // Get file size and divide file into blocks
            struct stat fs;
            if (fstat(fd, &fs) < 0) {
                perror("fstat");
                continue;
            }
            off_t fsize = fs.st_size;
            // cout << "file size: " << fsize << endl;
            int blocks = (int)fsize / FILE_BLOCK_MAX_SIZE;
            if (fsize % FILE_BLOCK_MAX_SIZE) blocks++;
            FileInfo *finfo = (FileInfo *)malloc(sizeof(*finfo) + sizeof(struct iovec) * blocks);
            finfo->fileSize = fsize;
            // Allocate an iovec for each block.
            for (int i = 0; i < blocks; i++) {
                size_t block_sz;
                if (i == blocks - 1 && (int)fsize % FILE_BLOCK_MAX_SIZE > 0)
                    block_sz = (int)fsize % FILE_BLOCK_MAX_SIZE;
                else
                    block_sz = FILE_BLOCK_MAX_SIZE;
                finfo->iovecs[i].iov_len = block_sz;
                void *buf;
                if (posix_memalign(&buf, FILE_BLOCK_MAX_SIZE, FILE_BLOCK_MAX_SIZE)) {
                    perror("posix_memalign");
                    continue;
                }
                finfo->iovecs[i].iov_base = buf;
            }
            // Submit event to SQE.
            URingEvent event;
            event.eventType = READ_FILE_EVENT;
            event.fileInfo = finfo;
            io_uring_sqe *sqe = io_uring_get_sqe(ring);
            io_uring_prep_readv(sqe, fd, finfo->iovecs, blocks, 0);
            io_uring_sqe_set_data(sqe, &event);
            io_uring_submit(ring);
            // cout << "Submit read file event to SQE success!" << endl;
            continue;
        }
    }
    return 0;
}

int main(int argc, const char *argv[]) {
    Reactor reactor;

    reactor.registerHandler(new Handlers::ReadFileHandler(FILE_HANDLER_NAME));
    reactor.registerHandler(new Handlers::SocketHandler(SOCKET_HANDLER_NAME));

    pthread_t listeningThread;
    pthread_create(&listeningThread, NULL, listenAndSubmitEvent, (void *)reactor.getIOring());

    reactor.run();

    return 0;
}