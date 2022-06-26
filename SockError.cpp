#include "SockError.hpp"
#include <cstring>
#ifdef _WIN32
#undef WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#endif
#else
#include <errno.h>
#endif

namespace SockWrapper {

    SockError GetLastError() {
        SockError error;
#ifndef _WIN32
        error.nErrorCode = errno;
        error.sErrorString = strerror(error.nErrorCode);
#else
        int errorCode = WSAGetLastError();
        char* s = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, WSAGetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&s, 0, NULL);
        fprintf(stderr, "%s\n", s);
        error.nErrorCode = errorCode;
        error.sErrorString = s;
#endif
        return error;
    }

}