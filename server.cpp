#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <thread>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define MAX_USERS 10


struct user_thread{
    SOCKET ClientSocket = INVALID_SOCKET;
    std::thread* t;
    int is_active = 0;
};

void user_thread_function(int thr_index, int max_users, struct user_thread* user_thread){
    int bytes_received, rec_buf_len = DEFAULT_BUFLEN;
    char rec_buf[DEFAULT_BUFLEN];
    while(1){
        if(bytes_received = recv(user_thread[thr_index].ClientSocket, rec_buf, rec_buf_len, 0) > 0){
            if(bytes_received == WSAENETRESET || bytes_received == WSAESHUTDOWN || bytes_received == WSAECONNABORTED){
                user_thread[thr_index].is_active = 0;
                printf("\nUser disconnected\n");
                delete user_thread[thr_index].t;
            }
            printf("%s\n",rec_buf);
            for(int i = 0; i < max_users;i++){
                if(i != thr_index){
                    send(user_thread[i].ClientSocket, rec_buf, rec_buf_len, 0 );
                }
            }
        }
    }
}
void connectNewUser(int* op_flag,int* number_of_clients,int max_users, SOCKET* ListenSocket, struct user_thread* thread_list){
    while(1){
        int iResult;
        SOCKET tmp;
        // Accept a client socket
        tmp = accept(*ListenSocket, NULL, NULL);
        *op_flag = 1;
        if (tmp == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(*ListenSocket);
            WSACleanup();
            exit(1);
        }
        int count = *number_of_clients;
        for(int i = 0; i < max_users;i++){
            if(thread_list[i].is_active == 0){
                thread_list[i].is_active = 1;
                thread_list[i].ClientSocket = tmp;
                thread_list[i].t = new std::thread(user_thread_function,i,max_users,thread_list);
                break;
            }

        }
        count++;
        *number_of_clients = count;
        printf("New client logged in\n");
        *op_flag = 0;
        }
    }

    // No longer need server socket
    // closesocket(*ListenSocket);

int __cdecl main(void) 
{
    WSADATA wsaData;
    int op_flag = 0;
    int bytes_received = 0;
    int counter = 0;
    int number_of_clients = 0;
    int iResult;
    int connection_flag=0;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;


    struct addrinfo *result = NULL;
    struct addrinfo hints;

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        exit(1);
    }
    struct user_thread* threads_array = (struct user_thread*)calloc(10,sizeof(struct user_thread));
// (int* op_flag,int* number_of_clients,int max_users, SOCKET* ListenSocket, struct user_thread*)

    std::thread t1(connectNewUser, &op_flag, &number_of_clients, MAX_USERS, &ListenSocket, threads_array);

    t1.join();
    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}