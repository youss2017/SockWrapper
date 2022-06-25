#include <iostream>
#include "SocketCore.hpp"

namespace Application
{
	using namespace std;
	using namespace SockWrapper;

	int Start(int argc, char** argv)
	{
		SockWrapper::Startup();

		Socket server(SocketType::TCP);
		server.Bind(1500, SocketInterface::Any).Listen(10);

		while (true) {
			try {
				Socket client = server.Accept();
				if (!client.IsConnectionAccepted())
					continue;
				client.SetBlockingMode(false);
			}
			catch (...) {}
		}

		SockWrapper::CleanUp();
		return 0;
	}

}