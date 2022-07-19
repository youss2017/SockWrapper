#pragma once
#include <string>
#include <cstdint>

namespace sw
{
    enum class SocketType
    {
        TCP,
        UDP,
        RAW
    };

    enum class SocketInterface {
        // 127.*.*.* packets only from your computer
        Loopback,
        // local network and internet
        Any,
        // binds to all interfaces and transmits through all of them
        // the best option for a server
        Broadcast
    };

    struct Endpoint {
        std::string mAddress;
        uint16_t mPort;

        static Endpoint GetEndPoint(const std::string& address, uint16_t port) {
            return { address, port };
        }

        static Endpoint GetEndPointBroadcast(uint16_t port);
    };

    class Socket
    {

    public:
        Socket(SocketType type);
        void CloseSocket();
        /// <summary>
        /// For more information on SocketInterface go to the enum class definition.
        /// </summary>
        /// <param name="port"></param>
        /// <param name="interfaceType"></param>
        /// <returns></returns>
        Socket &Bind(uint16_t port, SocketInterface interfaceType);
        Socket &Listen(int nMaxBacklog);
        Socket &Connect(const std::string &address, uint16_t port);

        Socket &Send(const void *pData, uint16_t size);
        Socket &SendTo(const void *pData, uint16_t size, const std::string &destIP, uint16_t destPort, int *pSentBytes = nullptr);
        Socket &SendTo(const void *pData, uint16_t size, const Endpoint& endpoint, int *pSentBytes = nullptr);

        // If in nonblocking mode pRecvBytes will be -1 until the os recieves the data.
        Socket &Recv(const void *pOutData, uint16_t size, int *pRecvBytes = nullptr);
        // sourceIP is your ip address
        Socket &RecvFrom(const void *pData, uint16_t size, int *pRecvBytes = nullptr, Endpoint* sourceEP = nullptr);

        Socket &SetBlockingMode(bool blocking);

        Socket Accept();

        const Endpoint& GetEndpoint();
        bool IsConnected();

        // Proper Disconnection.
        Socket &Disconnect();

    private:
        Socket() {}

    private:
        uint64_t mSocket;
        SocketType mType;
        Endpoint mEndpoint;
    };

}
