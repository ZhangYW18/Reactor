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

int FileHandler::handle(Request request) {
    std::cout << "FileHandler Handle" << std::endl;
    return 0;
}

int SocketHandler::handle(Request request) {
    std::cout << "SocketHandler Handle" << std::endl;
    return 0;
}

} // namespace Handlers