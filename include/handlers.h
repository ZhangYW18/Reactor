#ifndef REACTOR_HANDLERS_HANDLERS_H_
#define REACTOR_HANDLERS_HANDLERS_H_

#ifndef STRING
#define STRING
#include <string>
#endif

namespace Handlers {

struct Request {
};

class Handler {
public:
    Handler(std::string name);

    virtual ~Handler() {
    }

    virtual int handle(Request request) = 0;

    std::string getName();

protected:
    std::string name;
};

class FileHandler : public Handler {
    // Inherit all constructors of Handler class
    using Handler::Handler;

    int handle(Request request);
};

class SocketHandler : public Handler {
    // Inherit all constructors of Handler class
    using Handler::Handler;

    int handle(Request request);
};
} // namespace Handlers
#endif