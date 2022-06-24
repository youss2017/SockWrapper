#pragma once
#include <string>
#include <cstdint>

namespace SockWrapper
{
    enum class SocketType
    {
        TCP,
        UDP,
        RAW
    };

    struct Endpoint {
        std::string mAddress;
        uint16_t mPort;
    };

    class Socket
    {

    public:
        Socket(SocketType type);
        ~Socket();
        // Throws Exception on failure
        Socket &Bind(uint16_t port);
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
        bool IsConnectionAccepted();

        // Proper Disconnection.
        Socket &Disconnect();

    private:
        Socket() {}
        friend class SocketMultiplexing;

    private:
        bool mAccepted;
        uint64_t mSocket;
        SocketType mType;
        Endpoint mEndpoint;
    };

}