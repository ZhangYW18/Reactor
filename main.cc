#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
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
        /* Initialize socket */
        int ret = initSocket();
        if (ret != -1) {
            this->sock = ret;
        } else {
            fprintf(stderr, "init socket error");
        }
        memset(buf, 0, sizeof(buf));
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

    int getSocket() {
        return this->sock;
    }

    void registerHandler(Handlers::Handler *handler) {
        this->handlers[handler->getName()] = handler;
    }

    void run() {
        cout << "Reactor is running..." << endl;

        while (true) {
            io_uring_cqe *cqe;
            int ret = io_uring_wait_cqe(&ring, &cqe);
            if (ret < 0) {
                perror("io_uring_wait_cqe error");
                continue;
            }
            if (cqe->res < 0) {
                perror("io_uring_wait_cqe cqe res error");
                continue;
            }
            // cout << "ret: " << ret << endl;
            // cout << "res: " << cqe->res << endl;
            URingEvent *event;
            event = (URingEvent *)io_uring_cqe_get_data(cqe);
            // printf("address_cqe: %p\n", event->eventInfo.sockInfo);
            // printf("type2: %d\n", event->eventType);
            switch (event->eventType) {
            case READ_FILE_EVENT:
                ret = handlers[FILE_HANDLER_NAME]->handle(event, &ring, sock);
                break;
            case SOCKET_READ_EVENT:
                ret = handlers[SOCKET_READ_HANDLER_NAME]->handle(event, &ring, sock);
                break;
            case SOCKET_WRITE_EVENT:
                event->eventInfo.sockInfo->msgBuf = this->buf;
                ret = handlers[SOCKET_WRITE_HANDLER_NAME]->handle(event, &ring, sock);
                break;
            case EXIT_EVENT:
                io_uring_cqe_seen(&ring, cqe);
                return;
            default:
                fprintf(stderr, "Error: Unknown CQE event. EventType: %d\n", event->eventType);
                break;
            }
            if (ret < 0) {
                fprintf(stderr, "Handler handle error, code: %d", ret);
            }
            io_uring_cqe_seen(&ring, cqe);
        }
    }

private:
    unordered_map<string, Handlers::Handler *> handlers;
    struct io_uring ring;
    int sock;
    char buf[1024];

    int initSocket() {
        // Create a new socket.
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            fprintf(stderr, "socket error");
            return -1;
        }

        // Allow socket to reuse local address.
        int enable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))
            < 0) {
            fprintf(stderr, "setsockopt error");
            return -1;
        }

        // Assume echo server runs on 127.0.0.1:8000 and connect to it
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(SERVER_PORT);
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
            <= 0) {
            fprintf(stderr, "Invalid address/ Address not supported \n");
            return -1;
        }
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
            < 0) {
            fprintf(stderr, "Connection Failed \n");
            return -1;
        }

        // cout << "init socket success: " << sock << endl;
        return sock;
    }
};

struct thread_data {
    io_uring *ring;
    int sock;
};

void *listenAndSubmitEvent(void *args) {
    thread_data *data = (thread_data *)args;
    io_uring *ring = data->ring;
    int sock = data->sock;
    // bool connected = false;
    while (true) {
        string input;
        getline(cin, input);
        int space = input.find(" ");
        string command;
        if (space < 0)
            command = input;
        else
            command = input.substr(0, space);

        if (command == "cat") {
            // Get file path
            string tmp = input.substr(space + 1, input.length());
            char filePath[tmp.length() + 1];
            strcpy(filePath, tmp.c_str());
            // Open file
            int fd = open(filePath, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "Open file failed.\n");
                continue;
            }
            // Get file size and divide file into blocks
            struct stat fs;
            if (fstat(fd, &fs)
                < 0) {
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
            event.eventInfo.fileInfo = finfo;

            io_uring_sqe *sqe = io_uring_get_sqe(ring);
            io_uring_prep_readv(sqe, fd, finfo->iovecs, blocks, 0);
            io_uring_sqe_set_data(sqe, &event);
            io_uring_submit(ring);
            continue;
        }

        // Command echo xxx: send message "xxx" to echo server via socket and then read response from echo server.
        if (command == "echo") {
            // Get message and send it to echo server later
            string message = input.substr(space + 1, input.length());
            char *cstr = (char *)malloc(message.length() + 1);
            strcpy(cstr, message.c_str());
            // SockInfo *sinfo = new SockInfo();

            // Submit event to SQE.
            URingEvent event;
            event.eventType = SOCKET_WRITE_EVENT;
            event.eventInfo.sockInfo = new SockInfo();
            event.eventInfo.sockInfo->msgSize = strlen(cstr);

            struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
            io_uring_prep_send(sqe, sock, cstr, strlen(cstr), 0);
            io_uring_sqe_set_data(sqe, &event);
            io_uring_submit(ring);
            continue;
        }

        if (command == "exit") {
            URingEvent event;
            event.eventType = EXIT_EVENT;
            event.eventInfo.sockInfo = new SockInfo();
            struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
            io_uring_prep_close(sqe, sock);
            io_uring_sqe_set_data(sqe, &event);
            io_uring_submit(ring);
            continue;
        }

        cout << "Error: unknown command!" << endl;
    }
    return 0;
}

int main(int argc, const char *argv[]) {
    Reactor reactor;

    reactor.registerHandler(new Handlers::ReadFileHandler(FILE_HANDLER_NAME));
    reactor.registerHandler(new Handlers::SocketReadHandler(SOCKET_READ_HANDLER_NAME));
    reactor.registerHandler(new Handlers::SocketWriteHandler(SOCKET_WRITE_HANDLER_NAME));

    pthread_t listeningThread;
    thread_data data;
    data.ring = reactor.getIOring();
    data.sock = reactor.getSocket();
    pthread_create(&listeningThread, NULL, listenAndSubmitEvent, &data);

    reactor.run();

    return 0;
}