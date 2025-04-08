#include "Ws2tcpip.h" 
#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib,"ws2_32.lib")

#pragma warning (disable:4996)

unsigned char* DownloadData(size_t* pDataSize)
{
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in serverAddr;
    int response_size;
    const int chunkSize = 1024; 

    unsigned char* buffer = NULL;
    size_t totalSize = 0;
    size_t bufferSize = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed!\n");
        return NULL;
    }
    // Create socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        printf("Socket creation failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return NULL;
    }
    // Server info
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(443);
    if (InetPtonA(AF_INET, "<IP addr>", &serverAddr.sin_addr.s_addr) != 1) {
        printf("Invalid IP address format.\n");
        closesocket(s);
        WSACleanup();
        return NULL;
    }

    if (connect(s, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Error establishing connection with server\n");
        closesocket(s);
        WSACleanup();
        return NULL;
    }

    const char* httpRequest = "GET /data.bin HTTP/1.1\r\n"
        "Host: <IP addr>\r\n"
        "Connection: close\r\n\r\n";
    int bytesSent = send(s, httpRequest, (int)strlen(httpRequest), 0);
    if (bytesSent == SOCKET_ERROR) {
        printf("Send failed with error: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return NULL;
    }


    bufferSize = chunkSize;
    buffer = (unsigned char*)malloc(bufferSize);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        closesocket(s);
        WSACleanup();
        return NULL;
    }

    totalSize = 0;
    // 
    while (1) {
        if (totalSize + chunkSize > bufferSize) {
            bufferSize += chunkSize;
            unsigned char* newBuffer = (unsigned char*)realloc(buffer, bufferSize);
            if (newBuffer == NULL) {
                printf("Memory reallocation failed\n");
                free(buffer);
                closesocket(s);
                WSACleanup();
                return NULL;
            }
            buffer = newBuffer;
        }
        response_size = recv(s, (char*)(buffer + totalSize), chunkSize, 0);
        if (response_size == SOCKET_ERROR) {
            printf("Receiving data failed with error: %d\n", WSAGetLastError());
            break;
        }
        if (response_size == 0) { 
            break;
        }
        totalSize += response_size;
    }

    closesocket(s);
    WSACleanup();

    unsigned char* newBuffer = (unsigned char*)realloc(buffer, totalSize + 1);
    if (newBuffer != NULL) {
        buffer = newBuffer;
        buffer[totalSize] = '\0';
    }


    unsigned char* payload = NULL;
    const char* headerTerminator = "\r\n\r\n";
    payload = (unsigned char*)strstr((char*)buffer, headerTerminator);
    if (payload != NULL) {
        payload += strlen(headerTerminator);
        *pDataSize = totalSize - (payload - buffer);

        unsigned char* payloadCopy = (unsigned char*)malloc(*pDataSize + 1);
        if (payloadCopy != NULL) {
            memcpy(payloadCopy, payload, *pDataSize);
            payloadCopy[*pDataSize] = '\0';
            free(buffer);
            return payloadCopy;
        }
    }
    else {
        *pDataSize = totalSize;
        return buffer;
    }

    // error
    free(buffer);
    return NULL;
}
