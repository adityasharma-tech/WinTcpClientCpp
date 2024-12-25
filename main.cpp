#include "pch.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8002"
#define DEFAULT_BUFLEN 512

using namespace winrt;
using namespace Windows::Foundation;


int main()
{
    init_apartment();

    WSADATA wsaData;

    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    std::cout << "Initializing Winsock..." << std::endl;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    std::cout << "WSAStartup success." << std::endl;

    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo("192.168.99.154", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed: \n" << iResult << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;

	ptr = result;

    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
        ptr->ai_protocol);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cout << "Error at socket(): \n" << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Connect to server
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    // Should really try the next address returned by getaddrinfo
    // if the connect call failed
    // But for this simple example we just free the resources
    // returned by getaddrinfo and print an error message

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server!" << std::endl;
        WSACleanup();
        return 1;
    }

    char recvbuf[DEFAULT_BUFLEN];
    std::vector<char> buffer;

    int expectedLength = 0;

    // Receive data until the server closes the connection
    while (true) {
        int oResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);

        if (oResult > 0) {
            //std::cout << "Bytes received: " << oResult << std::endl;

            buffer.insert(buffer.end(), recvbuf, recvbuf + oResult);

            while (buffer.size() >= 4 || expectedLength > 0) {
                // If we don't know the length yet, read the first 4 bytes
                if (expectedLength == 0 && buffer.size() >= 4) {
                    expectedLength = ntohl(*reinterpret_cast<uint32_t*>(&buffer[0]));
                    buffer.erase(buffer.begin(), buffer.begin() + 4);
                }

                // If we have enough data for the complete frame
                if (expectedLength > 0 && buffer.size() >= expectedLength) {
                    std::vector<char> frame(buffer.begin(), buffer.begin() + expectedLength);
                    buffer.erase(buffer.begin(), buffer.begin() + expectedLength);

                    // Process the frame
                    std::cout << "Frame received of size: " << expectedLength << " bytes" << std::endl;

                    expectedLength = 0; // Reset for the next frame
                }
                else {
                    break; // Wait for more data
                }
            }
        }
        else if (oResult == 0) {
            std::cout << "Connection closed" << std::endl;
            break;
        }
        else {
            std::cout << "recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }  

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
