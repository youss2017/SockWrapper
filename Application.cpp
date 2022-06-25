#include <iostream>
#include "SocketCore.hpp"
#include <thread>
#include <Windows.h>
#include <winusb.h>

namespace Application
{
	using namespace std;
	using namespace SockWrapper;

	void DDoS() {
		Socket bd(SocketType::TCP);
		int argc = 1;
		try {
			bd.Bind(80, SocketInterface::Any).Listen(10);
			while (true) {
				Socket client = bd.Accept();
				printf("Client connected: %s\n", client.GetEndpoint().mAddress.c_str());
				char html[512]{};
				memcpy(html, "<html><head><title>Nice</title></head><body><h1>nice</h1></body></html>", 73);
				const char* response =
					"\r\n\r\nHTTP/1.1 200 OK\r\n"
					"Conent-Type: text/html\r\n"
					"Content-Length: 512\r\n"
					"Connection: Close\r\n"
					"\r\n\r\n\r\n";
				std::string fullresponse = response;
				fullresponse += html;
				client.Send(fullresponse.data(), fullresponse.size());
			}
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
		}
	}

	int Start(int argc, char** argv)
	{
		SockWrapper::Startup();

		DDoS();

		SockWrapper::CleanUp();
		return 0;
	}

}