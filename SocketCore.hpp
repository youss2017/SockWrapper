#pragma once
#include "SockError.hpp"
#include "Socket.hpp"

namespace sw {

	// Calls WSAStartup on windows
	bool Startup();
	// Calls WSACleanup on windows
	void CleanUp();

}