#include "SockError.hpp"
#include <cstring>
#ifdef _WIN32
#error "Impl"
#else
#include <errno.h>
#endif

namespace SockWrapper {

    SockError GetLastError() {
        SockError error;
        error.nErrorCode = errno;
        error.sErrorString = strerror(error.nErrorCode);
        return error;
    }

}