#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;
typedef float  f32;
typedef double f64;

#include "src/ship.c"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main()  {
  WSADATA wsaData;
  int iResult;

  SOCKET ListenSocket = INVALID_SOCKET;

  SOCKET ClientSocket[MAX_SHIPS] = {0};
  u8 socket_connection_index = 0;

  for(int i = 0; i < MAX_SHIPS; i++) {
    ClientSocket[i] = INVALID_SOCKET;
  }

  struct addrinfo *result = NULL;
  struct addrinfo hints;

  int iSendResult;
  
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
  if(iResult != 0) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  // Create a SOCKET for the server to listen for client connections.
  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if(ListenSocket == INVALID_SOCKET) {
    printf("socket failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // Setup the TCP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if(iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  freeaddrinfo(result);

  iResult = listen(ListenSocket, SOMAXCONN);
  if(iResult == SOCKET_ERROR) {
    printf("listen failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  do {
    while(socket_connection_index < MAX_SHIPS) {
      ClientSocket[socket_connection_index] = accept(ListenSocket, NULL, NULL);
      if(ClientSocket[socket_connection_index] == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
      } else {
        // printf("Conn Index %d\n", socket_connection_index);
        char* buffer = calloc(sizeof(char), 1);
        *buffer = socket_connection_index;
        int first_send_result = send(ClientSocket[socket_connection_index], buffer, 1, 0);
        if(first_send_result == SOCKET_ERROR) {
          printf("send failed with error: %d\n", WSAGetLastError());
          // return 1;
        } else {
          socket_connection_index++;
          if(socket_connection_index == MAX_SHIPS) {
            DWORD non_blocking = 1;
            if(ioctlsocket(ListenSocket, FIONBIO, &non_blocking) != 0) {
              printf("failed to set non-blocking on server\n");
            } else {
              printf("Non blocking on the server enabled\n");
            }
          }
        }
      }
    }

    for(u8 i = 0; i < MAX_SHIPS; i++) {
      #define RECEIVE_BUFFER_SIZE 9
      char receive_buffer[RECEIVE_BUFFER_SIZE];
      iResult = recv(ClientSocket[i], receive_buffer, RECEIVE_BUFFER_SIZE, 0);
      if(iResult > 0) {
        for(u8 j = 0; j < MAX_SHIPS; j++) {
          u8 player_id_sender = receive_buffer[0];
          if(player_id_sender != j) {
            iSendResult = send(ClientSocket[j], receive_buffer, RECEIVE_BUFFER_SIZE, 0);
            if(iSendResult == SOCKET_ERROR) {
              printf("send failed with error: %d\n", WSAGetLastError());
              return 1;
            }
          }
        }
      }
      else if(iResult == 0) {
        printf("Connection closing...\n");
      }
      else {
        printf("recv failed with error: %d\n", WSAGetLastError());
      }
    }
  } while(iResult > 0);

  return 0;
}