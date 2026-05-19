#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    
    char message[] = "Hello World from C client!";
    char response[1024];
    
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup error\n");
        return 1;
    }
    
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket error\n");
        WSACleanup();
        return 1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection error\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }
    
    send(client_socket, message, strlen(message), 0);
    printf("Sent: %s\n", message);
    
    recv(client_socket, response, sizeof(response), 0);
    printf("Received: %s\n", response);
    
    closesocket(client_socket);
    WSACleanup();
    
    return 0;
}