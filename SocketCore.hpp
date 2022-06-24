#pragma once
#include "Socket.hpp"
#include "SockError.hpp"

namespace SockWrapper {

    // Calls WSAStartup on windows
    bool Startup();
    // Calls WSACleanup on windows
    void CleanUp(); 

}