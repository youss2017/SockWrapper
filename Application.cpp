#include <iostream>
#include "SocketCore.hpp"
#include <thread>

namespace Application
{
    using namespace std;

    int Start(int argc, char **argv)
    {
        SockWrapper::Startup();
        try {
        
        SockWrapper::Socket s = SockWrapper::Socket(SockWrapper::SocketType::TCP).Bind(80).Listen(10);
        //std::thread([&]() {
            while(true) {
                auto client = s.Accept();
                printf("%d\n", client.IsConnectionAccepted());
                if (!client.IsConnectionAccepted())
                    continue;
                string html = "<html><body><h1>WebServer</h1></body></html>";
                string response = 
                "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-Length:"s + std::to_string(html.size()) + "\r\nConnection: Close\r\n\r\n"s + html;
                client.Send(response.data(), response.size());
                client.Disconnect();
            }
        //});

        } catch(std::exception& e) {
            cout << e.what() << endl;
        }

        cout << "Press any key (and enter) to exit." << endl;
        char buf[512];
        cin >> buf;
        
        SockWrapper::CleanUp();
        return 0;
    }

}