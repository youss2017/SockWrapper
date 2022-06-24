#pragma once
#include <string>

namespace SockWrapper {

    struct SockError {
        int nErrorCode;
        std::string sErrorString;
    };

    SockError GetLastError();

}
