#include <iostream>
#include <string>
#include <unordered_map>

#include "liburing.h"

#include "handlers.h"

using namespace std;

class Reactor {
public:
    Reactor() {
    }

    void registerHandler(Handlers::Handler *handler) {
        this->handlers[handler->getName()] = handler;
        Handlers::Request request;
        // int res = this->handlers[handler->getName()]->handle(request);
        // cout << res << endl;
    }

    void run() {
        struct io_uring_cqe *cqe;
        cout << "Reactor is running..." << endl;
        string s;
        /*
        while (true) {

            int ret = io_uring_wait_cqe(&ring, &cqe);
            if (ret < 0) {
                std::cout << "io_uring_wait_cqe error" << std::endl;
                continue;
            }
            if (cqe->res < 0) {
                std::cout << "io_uring_wait_cqe cqe res error" << std::endl;
                continue;
            }

            // struct request *req = (struct request *)cqe->user_data;

        }
        */
    }

private:
    std::unordered_map<std::string, Handlers::Handler *> handlers;
};

int main(int argc, const char *argv[]) {
    Reactor reactor;

    Handlers::Handler *fileHandler = new Handlers::FileHandler("file");
    reactor.registerHandler(fileHandler);
    Handlers::Handler *socketHandler = new Handlers::SocketHandler("socket");
    reactor.registerHandler(socketHandler);

    // open one thread to run reactor
    reactor.run();

    // Open another thread to send request

    return 0;
}