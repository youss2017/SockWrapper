#pragma once
#include <string>

namespace sw {

	struct SockError {
		int nErrorCode;
		std::string sErrorString;
	};

	SockError GetLastError();

}