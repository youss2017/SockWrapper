#include <iostream>
#include "SocketCore.hpp"
#include <cstring>

namespace Application
{
	using namespace std;

	int Start(int argc, char** argv)
	{
		sw::Startup();

		sw::Socket spam(sw::SocketType::UDP);
		spam.Bind(443, sw::SocketInterface::Any);
		while (true)
		{
			char randomData[1024];
			// Open Task Manager to see Network Traffic Spike.
			strcpy(randomData, "SPAM UDP Packet! You can see this in wireshark! Broadcast address based on adapter.");
			spam.SendTo(randomData, 1024, sw::Endpoint::GetEndPointBroadcast(443));
		}

		sw::Socket server(sw::SocketType::TCP);
		server.Bind(1500, sw::SocketInterface::Any).Listen(10);

		while (true)
		{
			try
			{
				sw::Socket client = server.Accept();
				client.SetBlockingMode(false);
			}
			catch (std::exception& e)
			{
				cout << "client could not be accepted because '" << e.what() << "'" << endl;
			}
		}

		sw::CleanUp();
		return 0;
	}

}
