
#include <bits/stdc++.h>

#include "liburing.h"

using namespace std;

class Reactor {
public:
    Reactor() {
    }

    void run() {
        struct io_uring_cqe *cqe;
        struct
    }

private:
    unordered_map<string, Handler> handlers;
};

class Handler {
public:
    int handle_event(string eventType) {
        cout << "Handle Event " << eventType << endl;
        return 0;
    }
};

int main() {
    return 0;
}