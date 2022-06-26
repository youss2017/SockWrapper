#include "Socket.hpp"
#include "SockError.hpp"
#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <cstring>
#else
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#endif
#include <iostream>
#include <sstream>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")
typedef int socklen_t;
#endif

namespace SockWrapper
{

	static void Socket_ThrowException(bool invalidArgs = false, const char* detailedError = nullptr)
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
		if (type == SocketType::RAW)
		{
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
			// domain = AF_PACKET;
			// ipproto = htons(ETH_P_ALL);
			typeInt = SOCK_RAW;
			break;
		default:
			Socket_ThrowException(true);
		}
		mSocket = ::socket(domain, typeInt, ipproto);
		mType = type;
		mEndpoint.mPort = 0;
		mEndpoint.mAddress = "0.0.0.0";
		if (mSocket < 0)
			Socket_ThrowException();
	}

	Socket::Socket(Socket&& move)
	{
		if (this == &move)
			return;
		this->mEndpoint = move.mEndpoint;
		this->mSocket = move.mSocket;
		this->mType = move.mType;
		move.mSocket = -1;
	}

	Socket::~Socket()
	{
		Disconnect();
#ifdef _WIN32
		::closesocket(mSocket);
#else
		::close(mSocket);
#endif
	}

	static ::in_addr GetIPAddress(bool adapaterAddressFlag, bool subnetAddressFlag, bool broadcastAddressFlag, int* pOutSubnetBits = nullptr)
	{
		static ::in_addr adapterAddress;
		static ::in_addr subnetAddress;
		static ::in_addr broadcastAddress;
		static int subnetBits;
		static bool initalize = true;
		if (initalize)
		{
			initalize = false;
#ifdef _WIN32
			ULONG size = 0;
			GetAdaptersInfo(nullptr, &size);
			PIP_ADAPTER_INFO ip = (PIP_ADAPTER_INFO)malloc(size);
			void* allocationAddress = ip;
			GetAdaptersInfo(ip, &size);
			while (true)
			{
				if (ip->Type & (IF_TYPE_IEEE80211 | MIB_IF_TYPE_ETHERNET))
					if (strcmp(ip->IpAddressList.IpAddress.String, "0.0.0.0") != 0 &&
						strcmp(ip->GatewayList.IpAddress.String, "0.0.0.0") != 0 &&
						strcmp(ip->DhcpServer.IpAddress.String, "0.0.0.0") != 0)
					{
						break;
					}
				ip = ip->Next;
				if (!ip)
					break;
			}
			if (ip != NULL)
			{
				uint32_t adapterAddressInt = inet_addr(ip->IpAddressList.IpAddress.String);
				uint32_t subnetAddressInt = inet_addr(ip->IpAddressList.IpMask.String);
				subnetBits =
					(((subnetAddressInt >> 24) == 0x00) ? 8 : 0) +
					(((subnetAddressInt >> 16) == 0x00) ? 8 : 0) +
					(((subnetAddressInt >> 8) == 0x00) ? 8 : 0) +
					(((subnetAddressInt >> 0) == 0x00) ? 8 : 0);
				unsigned char broadcastAddressInt[4];
				broadcastAddressInt[3] = subnetBits >= 8 ? 255 : (adapterAddressInt >> 24) & 0xff;
				broadcastAddressInt[2] = subnetBits >= 16 ? 255 : (adapterAddressInt >> 16) & 0xff;
				broadcastAddressInt[1] = subnetBits >= 24 ? 255 : (adapterAddressInt >> 8) & 0xff;
				broadcastAddressInt[0] = subnetBits >= 32 ? 255 : (adapterAddressInt >> 0) & 0xff;
				memcpy(&adapterAddress, &adapterAddressInt, sizeof(uint32_t));
				memcpy(&subnetAddress, &subnetAddressInt, sizeof(uint32_t));
				memcpy(&broadcastAddress, &broadcastAddressInt, sizeof(uint32_t));
			}
			free(allocationAddress);
#else
			// from https://stackoverflow.com/questions/18100761/obtaining-subnetmask-in-c
			::ifaddrs* ifap;
			::getifaddrs(&ifap);
			for (::ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next)
			{
				if (ifa->ifa_addr->sa_family == AF_INET)
				{
					std::string address = inet_ntoa(((sockaddr_in*)ifa->ifa_addr)->sin_addr);
					std::string netmask = inet_ntoa(((sockaddr_in*)ifa->ifa_netmask)->sin_addr);
					if (strcmp(address.c_str(), "127.0.0.1") != 0 && strcmp(address.c_str(), "0.0.0.0") != 0)
					{
						uint32_t adapterAddressInt = ((::sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
						uint32_t subnetAddressInt = ((::sockaddr_in*)ifa->ifa_netmask)->sin_addr.s_addr;
						subnetBits =
							(((subnetAddressInt >> 24) == 0x00) ? 8 : 0) +
							(((subnetAddressInt >> 16) == 0x00) ? 8 : 0) +
							(((subnetAddressInt >> 8) == 0x00) ? 8 : 0) +
							(((subnetAddressInt >> 0) == 0x00) ? 8 : 0);
						unsigned char broadcastAddressInt[4];
						broadcastAddressInt[3] = subnetBits >= 8 ? 255 : (adapterAddressInt >> 24) & 0xff;
						broadcastAddressInt[2] = subnetBits >= 16 ? 255 : (adapterAddressInt >> 16) & 0xff;
						broadcastAddressInt[1] = subnetBits >= 24 ? 255 : (adapterAddressInt >> 8) & 0xff;
						broadcastAddressInt[0] = subnetBits >= 32 ? 255 : (adapterAddressInt >> 0) & 0xff;
						memcpy(&adapterAddress, &adapterAddressInt, sizeof(uint32_t));
						memcpy(&subnetAddress, &subnetAddressInt, sizeof(uint32_t));
						memcpy(&broadcastAddress, &broadcastAddressInt, sizeof(uint32_t));
						break;
					}
				}
			}
			::freeifaddrs(ifap);
#endif
		}
		if (pOutSubnetBits)
			*pOutSubnetBits = subnetBits;
		if (adapaterAddressFlag)
		{
			return adapterAddress;
		}
		if (subnetAddressFlag)
		{
			return subnetAddress;
		}
		if (broadcastAddressFlag)
		{
			return broadcastAddress;
		}
		return adapterAddress;
	}

	// Throws Exception on failure
	Socket& Socket::Bind(uint16_t port, SocketInterface interfaceType)
	{
		sockaddr_in addr;
		switch (interfaceType)
		{
		case SocketInterface::Loopback:
			addr.sin_addr.s_addr = INADDR_LOOPBACK;
			break;
		case SocketInterface::Any:
			addr.sin_addr.s_addr = INADDR_ANY;
			break;
		case SocketInterface::Broadcast:
			addr.sin_addr.s_addr = INADDR_BROADCAST;
			break;
		}
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		mEndpoint.mPort = port;
		if (::bind(mSocket, (sockaddr*)&addr, sizeof(addr)) < 0)
		{
			Socket_ThrowException();
		}
		return *this;
	}

	Socket& Socket::Listen(int nMaxBacklog)
	{
		GetIPAddress(false, false, false);
		if (mType != SocketType::TCP)
			Socket_ThrowException(true, "Must be TCP Socket to use Listen Function()");
		if (::listen(mSocket, nMaxBacklog) < 0)
			Socket_ThrowException();
		return *this;
	}

	Socket& Socket::Connect(const std::string& address, uint16_t port)
	{
		sockaddr_in addr{};
		addr.sin_addr.s_addr = inet_addr(address.c_str());
		addr.sin_port = htons(port);
		addr.sin_family = AF_INET;
		if (::connect(mSocket, (sockaddr*)&addr, sizeof(addr)) < 0)
		{
			Socket_ThrowException();
		}
		return *this;
	}

	Socket& Socket::Send(const void* pData, uint16_t size)
	{
		if (mType != SocketType::TCP)
			Socket_ThrowException(true, "Must be TCP Socket to use Send Function()");
		if (::send(mSocket, (const char*)pData, size, 0) < 0)
			Socket_ThrowException();
		return *this;
	}

	Socket& Socket::SendTo(const void* pData, uint16_t size, const std::string& destIP, uint16_t destPort, int* pSentBytes)
	{
		if (mType == SocketType::TCP)
			Socket_ThrowException(true, "Must be UDP/RAW Socket to use Send Function()");
		sockaddr_in dest{};
		dest.sin_addr.s_addr = inet_addr(destIP.c_str());
		dest.sin_family = AF_INET;
		dest.sin_port = htons(destPort);
		socklen_t len = sizeof(dest);
		int sentBytes = ::sendto(mSocket, (const char*)pData, size, 0, (sockaddr*)&dest, len);
		if (sentBytes < 0)
			Socket_ThrowException();
		if (pSentBytes)
			*pSentBytes = sentBytes;
		return *this;
	}

	Socket& Socket::SendTo(const void* pData, uint16_t size, const Endpoint& endpoint, int* pSentBytes)
	{
		if (mType == SocketType::TCP)
			Socket_ThrowException(true, "Must be UDP/RAW Socket to use Send Function()");
		u_long broadcastEnable = 1;
		::setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcastEnable, sizeof(broadcastEnable));
		sockaddr_in dest{};
		dest.sin_addr.s_addr = inet_addr(endpoint.mAddress.c_str());
		dest.sin_family = AF_INET;
		dest.sin_port = htons(endpoint.mPort);
		socklen_t len = sizeof(dest);
		int sentBytes = ::sendto(mSocket, (const char*)pData, size, 0, (sockaddr*)&dest, len);
		if (sentBytes < 0)
			Socket_ThrowException();
		if (pSentBytes)
			*pSentBytes = sentBytes;
		return *this;
	}

	Socket& Socket::Recv(const void* pOutData, uint16_t size, int* pRecvBytes)
	{
		if (mType != SocketType::TCP)
			Socket_ThrowException(true, "Must be TCP Socket to use Recv Function()");
		int recvBytes = ::recv(mSocket, (char*)pOutData, size, 0);
		int eCode = errno;
		if (!(eCode == EAGAIN || eCode == EWOULDBLOCK) && recvBytes < 0)
		{
			Socket_ThrowException();
		}
		if (pRecvBytes)
			*pRecvBytes = recvBytes;
		return *this;
	}

	Socket& Socket::RecvFrom(const void* pData, uint16_t size, int* pRecvBytes, Endpoint* sourceEP)
	{
		if (mType == SocketType::TCP)
			Socket_ThrowException(true, "Must be UDP/RAW Socket to use RecvFrom Function()");
		sockaddr_in src{};
		socklen_t len = sizeof(src);
		int recvBytes = ::recvfrom(mSocket, (char*)pData, size, 0, (sockaddr*)&src, &len);
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

	Socket& Socket::SetBlockingMode(bool blocking)
	{
		if (mType == SocketType::RAW)
			Socket_ThrowException(true, "Cannot set blocking mode on RAW Socket");
		u_long code = blocking ? 0 : 1;
#ifdef _WIN32
		if (::ioctlsocket(mSocket, FIONBIO, &code) < 0)
			Socket_ThrowException();
#else
		if (::ioctl(mSocket, FIONBIO, &code) < 0)
			Socket_ThrowException();
#endif
		return *this;
	}

	Socket Socket::Accept()
	{
		if (mType != SocketType::TCP)
			Socket_ThrowException(true, "Must be TCP Socket to use Accept Function()");
		sockaddr_in addrs{};
		socklen_t len = sizeof(sockaddr_in);
		uint64_t client = (uint64_t)::accept(mSocket, (sockaddr*)&addrs, &len);
		Socket s;
		if (client < 0)
			Socket_ThrowException();
		s.mType = SocketType::TCP;
		s.mSocket = client;
		s.mEndpoint.mAddress = inet_ntoa(addrs.sin_addr);
		s.mEndpoint.mPort = ntohs(addrs.sin_port);
		return s;
	}

	const Endpoint& Socket::GetEndpoint()
	{
		return mEndpoint;
	}

	bool Socket::IsConnected()
	{
		if (mType != SocketType::TCP)
			Socket_ThrowException(true, "IsConnected() is only allowed when using TCP connections.");
		try
		{
			char data[1];
			Recv(data, 0, nullptr);
			return true;
		}
		catch (std::exception& e)
		{
			return false;
		}
	}

	// Proper Disconnection.
	Socket& Socket::Disconnect()
	{
#ifdef _WIN32
		::shutdown(mSocket, SD_BOTH);
#else
		::shutdown(mSocket, SHUT_RDWR);
#endif
		return *this;
	}
	Endpoint Endpoint::GetEndPointBroadcast(uint16_t port)
	{
		Endpoint ep;
		ep.mAddress = inet_ntoa(GetIPAddress(false, false, true));
		ep.mPort = port;
		return ep;
	}

}
