#include "Socket.hpp"
#include "SockError.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>

namespace SockWrapper
{

    static void Socket_ThrowException(bool invalidArgs = false, const char *detailedError = nullptr)
    {
        std::stringstream ss;
        if (!invalidArgs)
        {
            auto error = GetLastError();
            ss << "Error Code: " << error.nErrorCode << ", Error String: " << error.sErrorString;
        }
        else
        {
            if (detailedError)
            {
                ss << detailedError;
            }
            else
            {
                ss << "Invalid Arguments";
            }
        }
        throw std::runtime_error(ss.str());
    }

    Socket::Socket(SocketType type)
    {
        #ifdef _WIN32
        if(type == SocketType::RAW) {
            Socket_ThrowException(true, "Raw Sockets on windows not implemented. RAW Socket on Windows Need WinPcap");
        }
        #endif
        int domain = AF_INET;
        int typeInt;
        int ipproto;
        switch (type)
        {
        case SocketType::TCP:
            ipproto = IPPROTO_TCP;
            typeInt = SOCK_STREAM;
            break;
        case SocketType::UDP:
            ipproto = IPPROTO_UDP;
            typeInt = SOCK_DGRAM;
            break;
        case SocketType::RAW:
            domain = AF_PACKET;
            ipproto = htons(ETH_P_ALL);
            typeInt = SOCK_RAW;
            break;
        default:
            Socket_ThrowException(true);
        }
        mSocket = ::socket(domain, typeInt, ipproto);
        mType = type;
        mEndpoint.mPort = 0;
        mEndpoint.mAddress = "0.0.0.0";
        mAccepted = true;
        if (mSocket < 0)
            Socket_ThrowException();
    }

    Socket::~Socket()
    {
        Disconnect();
        ::close(mSocket);
    }

    // Throws Exception on failure
    Socket &Socket::Bind(uint16_t port)
    {
        sockaddr_in addr;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        mEndpoint.mPort = port;
        if (::bind(mSocket, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            Socket_ThrowException();
        }
        return *this;
    }

    Socket &Socket::Listen(int nMaxBacklog)
    {
        if (mType != SocketType::TCP)
            Socket_ThrowException(true, "Must be TCP Socket to use Listen Function()");
        if (::listen(mSocket, nMaxBacklog) < 0)
            Socket_ThrowException();
        return *this;
    }

    Socket &Socket::Connect(const std::string &address, uint16_t port)
    {
        sockaddr_in addr{};
        addr.sin_addr.s_addr = inet_addr(address.c_str());
        addr.sin_port = htons(port);
        addr.sin_family = AF_INET;
        if (::connect(mSocket, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            Socket_ThrowException();
        }
        return *this;
    }

    Socket &Socket::Send(const void *pData, uint16_t size)
    {
        if (mType != SocketType::TCP)
            Socket_ThrowException(true, "Must be TCP Socket to use Send Function()");
        if (::send(mSocket, pData, size, 0) < 0)
            Socket_ThrowException();
        return *this;
    }

    Socket &Socket::SendTo(const void *pData, uint16_t size, const std::string &destIP, uint16_t destPort, int *pSentBytes)
    {
        if (mType == SocketType::TCP)
            Socket_ThrowException(true, "Must be UDP/RAW Socket to use Send Function()");
        sockaddr_in dest{};
        dest.sin_addr.s_addr = inet_addr(destIP.c_str());
        dest.sin_family = AF_INET;
        dest.sin_port = htons(destPort);
        socklen_t len = sizeof(dest);
        int sentBytes = ::sendto(mSocket, pData, size, 0, (sockaddr *)&dest, len);
        if (sentBytes < 0)
            Socket_ThrowException();
        if (pSentBytes)
            *pSentBytes = sentBytes;
        return *this;
    }

    Socket &Socket::SendTo(const void *pData, uint16_t size, const Endpoint &endpoint, int *pSentBytes)
    {
        if (mType == SocketType::TCP)
            Socket_ThrowException(true, "Must be UDP/RAW Socket to use Send Function()");
        sockaddr_in dest{};
        dest.sin_addr.s_addr = inet_addr(endpoint.mAddress.c_str());
        dest.sin_family = AF_INET;
        dest.sin_port = htons(endpoint.mPort);
        socklen_t len = sizeof(dest);
        int sentBytes = ::sendto(mSocket, pData, size, 0, (sockaddr *)&dest, len);
        if (sentBytes < 0)
            Socket_ThrowException();
        if (pSentBytes)
            *pSentBytes = sentBytes;
        return *this;
    }

    Socket &Socket::Recv(const void *pOutData, uint16_t size, int *pRecvBytes)
    {
        if (mType != SocketType::TCP)
            Socket_ThrowException(true, "Must be TCP Socket to use Recv Function()");
        int recvBytes = ::recv(mSocket, (void *)pOutData, size, 0);
        int eCode = errno;
        if (!(eCode == EAGAIN || eCode == EWOULDBLOCK) && recvBytes < 0)
        {
            Socket_ThrowException();
        }
        if (pRecvBytes)
            *pRecvBytes = recvBytes;
        return *this;
    }

    Socket &Socket::RecvFrom(const void *pData, uint16_t size, int *pRecvBytes, Endpoint *sourceEP)
    {
        if (mType == SocketType::TCP)
            Socket_ThrowException(true, "Must be UDP/RAW Socket to use RecvFrom Function()");
        sockaddr_in src{};
        socklen_t len = sizeof(src);
        int recvBytes = ::recvfrom(mSocket, (void *)pData, size, 0, (sockaddr *)&src, &len);
        if (recvBytes < 0)
            Socket_ThrowException();
        if (pRecvBytes)
            *pRecvBytes = recvBytes;
        if (sourceEP)
        {
            sourceEP->mAddress = inet_ntoa(src.sin_addr);
            sourceEP->mPort = ntohs(src.sin_port);
        }
        return *this;
    }

    Socket &Socket::SetBlockingMode(bool blocking)
    {
        if(mType == SocketType::RAW)
            Socket_ThrowException(true, "Cannot set blocking mode on RAW Socket");
        int code = blocking ? 0 : 1;
        if (::ioctl(mSocket, FIONBIO, &code) < 0)
            Socket_ThrowException();
        return *this;
    }

    Socket Socket::Accept()
    {
        if (mType != SocketType::TCP)
            Socket_ThrowException(true, "Must be TCP Socket to use Accept Function()");
        sockaddr_in addrs{};
        socklen_t len = sizeof(sockaddr_in);
        int client = ::accept(mSocket, (sockaddr *)&addrs, &len);
        Socket s;
        if (client < 0)
            s.mAccepted = false;
        else
            s.mAccepted = true;
        s.mType = SocketType::TCP;
        s.mSocket = client;
        s.mEndpoint.mAddress = inet_ntoa(addrs.sin_addr);
        s.mEndpoint.mPort = ntohs(addrs.sin_port);
        return s;
    }

    const Endpoint &Socket::GetEndpoint()
    {
        return mEndpoint;
    }

    bool Socket::IsConnectionAccepted()
    {
        return mAccepted;
    }

    // Proper Disconnection.
    Socket &Socket::Disconnect()
    {
        ::shutdown(mSocket, SHUT_RDWR);
        return *this;
    }
}