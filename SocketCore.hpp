#pragma once
#include "SockError.hpp"
#include "Socket.hpp"

namespace SockWrapper {

	// Calls WSAStartup on windows
	bool Startup();
	// Calls WSACleanup on windows
	void CleanUp();

}