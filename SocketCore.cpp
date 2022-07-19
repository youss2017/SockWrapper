#ifdef _WIN32
#include <Windows.h>
#include <WinSock2.h>
#endif

namespace sw {
    
    bool Startup() {
        #ifdef _WIN32
        WSADATA ws;
        return 0 == WSAStartup(MAKEWORD(2, 2), &ws);
        #endif
        return true;
    }
    
    void CleanUp() {
        #ifdef _WIN32
        WSACleanup();
        #endif
    } 

}