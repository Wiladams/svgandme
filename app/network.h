#pragma once


//
//References:
//https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedInternet3a.html
//

#include <cstring>
#include <cstdio>
#include <vector>

#define WINSOCK_DEPRECATED_NO_WARNINGS 1
#define WIN32_LEAN_AND_MEAN
#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")


// Implementation of network byte ordering functions
// These routines are typically provided by the OS
// these routines here just show you how they can be implemented
/*
namespace IPUtils {

inline int16_t htons(int16_t value) {return isLE() ? swapUInt16((uint16_t)value) : value;}
inline int16_t ntohs(int16_t value) {return isLE() ? swapUInt16((uint16_t)value) : value;}

inline int32_t htonl(int32_t value) {return isLE() ? swapUInt32((uint32_t)value) : value;}
inline int32_t ntohl(int32_t value) {return isLE() ? swapUInt32((uint32_t)value) : value;}

};
*/


namespace waavs {
    
    //    This should live somewhere else, higher in the stack
    
    struct BufferChunk
    {
        uint8_t* fData;
        size_t fSize;
        bool fIOwnData;

        BufferChunk(void* data, const size_t size)
            :fData((uint8_t*)data),
            fSize(size),
            fIOwnData(false)
        {
        }

        BufferChunk(const size_t size)
        {
            fData = { new uint8_t[size]{} };
            fSize = size;
            fIOwnData = true;
        }

        ~BufferChunk()
        {
            if (fIOwnData) {
                delete fData;
            }
            fSize = 0;
        }

        void* data() { return fData; }
        size_t size() { return fSize; }
    };


    struct IPAddress {
    private:
        /*
            IPAddress()
            {
                memset(&fAddress, 0, sizeof(fAddress));
                fAddressLength = 0;
            }
        */
    public:
        struct sockaddr* fAddress = nullptr;
        size_t fAddressLength = 0;

        // Construct from traditional sockaddr and length
        IPAddress(const struct sockaddr* addr, const size_t addrLen)
        {
            fAddress = (struct sockaddr*)malloc(addrLen);
            if (nullptr == fAddress)
                return;

            memcpy(fAddress, addr, addrLen);
            fAddressLength = addrLen;
        }

        ~IPAddress()
        {
            free(fAddress);
        }


        int toString(char* addressBuff, int addressBuffLen)
        {
            DWORD consumedLength = addressBuffLen;
            WSAAddressToStringA(fAddress, (DWORD)fAddressLength, nullptr, addressBuff, &consumedLength);

            return consumedLength;
        }

    };

    class IPHost {
        char fHostName[256]{};
        std::vector<IPAddress*> fAddresses{};
        std::vector<char*> fAliases{};

    public:


        void setName(const char* name)
        {
            memset(fHostName, 0, sizeof(fHostName));
            
            if (name != nullptr) {
                strncpy_s(fHostName, sizeof(fHostName), name, sizeof(fHostName));

            }
        }
        const char* getName() const { return fHostName; }

        // addAddress
        bool addAddress(const struct sockaddr* addr, const size_t addrlen)
        {
            fAddresses.push_back(new IPAddress(addr, addrlen));

            return true;
        }

        size_t numberOfAddresses() { return fAddresses.size(); }

        IPAddress* getAddress(const int idx = 0)
        {
            return fAddresses[idx];
        }

        // addAlias
        bool addAlias(const char* )
        {
            return false;
        }

        static IPHost* create(const char* hostname, const char* portname, int family = AF_INET, int socktype = SOCK_STREAM, bool isnumericstring = false)
        {
            int err;
            addrinfo hints;
            struct addrinfo* ppResult;

            // need to zero out some members before using
            memset(&hints, 0, sizeof(hints));

            hints.ai_family = family;
            hints.ai_socktype = socktype;
            hints.ai_protocol = IPPROTO_TCP;
            hints.ai_flags = AI_CANONNAME;    // hostname is a canonical name

            if (isnumericstring) {
                hints.ai_flags = AI_NUMERICHOST;
            }

            err = getaddrinfo(hostname, portname, &hints, &ppResult);

            if (err != 0) {
                printf("IPHost.create(), getaddrinfo ERROR: %d\n", WSAGetLastError());
                return nullptr;
            }

            // Create the host
            IPHost* host = new IPHost();
            // Add initial name and address
            host->setName(ppResult->ai_canonname);
            host->addAddress(ppResult->ai_addr, ppResult->ai_addrlen);

            // go through the rest of the entries
            // and add their information as well
            struct addrinfo* sentinel = ppResult->ai_next;
            while (sentinel != nullptr) {
                if (sentinel->ai_canonname != nullptr)
                {
                    host->addAlias(sentinel->ai_canonname);
                }

                host->addAddress(sentinel->ai_addr, sentinel->ai_addrlen);
                sentinel = sentinel->ai_next;
            }

            // free the addrinfos structure
            freeaddrinfo(ppResult);

            return host;
        }
    };

    class IPSocket {
    public:
        SOCKET fSocket;

    private:
        bool fIsValid = false;
        int fLastError = 0;
        bool fAutoClose = false;

    public:
        // Construct with an existing native socket
        IPSocket(SOCKET s, const bool autoclose = false)
            : fSocket(s),
            fLastError(0),
            fAutoClose(autoclose)
        {
            fIsValid = (s != INVALID_SOCKET);
        }

        // Default constructor will initially be invalid
        IPSocket()
            : IPSocket(INVALID_SOCKET)
        {
        }



        // Construct a particular kind of socket
        IPSocket(int family, int socktype, int protocol = 0)
            : fIsValid(false)
        {
            fSocket = WSASocketA(family, socktype, protocol, nullptr, 0, 0);

            if (fSocket == INVALID_SOCKET) {
                printf("INVALID_SOCKET: %Id\n", fSocket);
                fLastError = WSAGetLastError();
                return;
            }

            fIsValid = true;
        }

        // There should be a flag to autoclose
        // otherwise, if you create one of these on the 
        // stack, or copy it, it will close
        virtual ~IPSocket() {
            if (fAutoClose) {
                close();
            }
        }

        bool isValid() const { return fIsValid; }
        int getLastError() const { return fLastError; }

        IPSocket accept()
        {
            //struct sockaddr clientAddr {};
            //int clientAddrLen=0;
            //int res = ::accept(fSocket,&clientAddr,&clientAddrLen);
            SOCKET res = ::accept(fSocket, nullptr, nullptr);

            if (res == INVALID_SOCKET) {
                fLastError = WSAGetLastError();
                return IPSocket();
            }

            return IPSocket(res);
        }

        int bindTo(const sockaddr* addr, const int addrLen)
        {
            return ::bind(fSocket, addr, addrLen);
        }

        // Closing a socket should include a shutdown
        // so the socket isn't lingering
        bool close() {
            int result = ::closesocket(fSocket);
            if (result != 0) {
                fLastError = WSAGetLastError();
                return false;
            }

            return true;
        }

        bool listen(int backlog = 5)
        {
            int result = ::listen(fSocket, backlog);
            if (result != 0) {
                fLastError = WSAGetLastError();
                return false;
            }

            return true;
        }


        // Send to a specific address
        // The address was specified when we 
        // created the socket
        int sendTo(const struct sockaddr* addrTo, int addrToLen, const char* buff, const int bufflen)
        {
            return ::sendto(fSocket, buff, bufflen, 0, addrTo, addrToLen);
        }

        int receiveFrom(struct sockaddr* addrFrom, int* addrFromLen, char* buff, int bufflen)
        {
            return ::recvfrom(fSocket, buff, bufflen, 0, addrFrom, addrFromLen);
        }

        // Send a chunk of memory
        // return the number of octets sent
        int send(const char* buff, int len, int flags = 0)
        {
            return ::send(fSocket, buff, len, flags);
        }

        int sendChunk(BufferChunk& chunk, int flags = 0)
        {
            return this->send((char*)chunk.fData, (int)chunk.fSize, flags);
        }

        // receive a chunk of memmory
        // return number of octets received
        int receive(char* buff, int len, int flags = 0)
        {
            return ::recv(fSocket, buff, len, flags);
        }

        int receiveChunk(BufferChunk& chunk, int flags = 0)
        {
            int retCode = ::recv(fSocket, (char*)chunk.fData, (int)chunk.fSize, flags);
            return retCode;
        }
    };
}

