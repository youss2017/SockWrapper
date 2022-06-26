#include <iostream>
#include "SocketCore.hpp"
#include <cstring>

namespace Application
{
	using namespace std;
	using namespace SockWrapper;

	int Start(int argc, char** argv)
	{
		SockWrapper::Startup();

		Socket spam(SocketType::UDP);
		spam.Bind(443, SocketInterface::Any);
		while (true)
		{
			char randomData[1024];
			// Open Task Manager to see Network Traffic Spike.
			strcpy(randomData, "SPAM UDP Packet! You can see this in wireshark! Broadcast address based on adapter.");
			spam.SendTo(randomData, 1024, Endpoint::GetEndPointBroadcast(443));
		}

		Socket server(SocketType::TCP);
		server.Bind(1500, SocketInterface::Any).Listen(10);

		while (true)
		{
			try
			{
				Socket client = server.Accept();
				client.SetBlockingMode(false);
			}
			catch (std::exception& e)
			{
				cout << "client could not be accepted because '" << e.what() << "'" << endl;
			}
		}

		SockWrapper::CleanUp();
		return 0;
	}

}
