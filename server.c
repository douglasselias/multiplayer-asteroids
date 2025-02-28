#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct addrinfo addrinfo;

#include "src/types.c"
#include "src/ship.c"
#include "src/socket.c"

#pragma comment (lib, "Ws2_32.lib")

s32 main()  {
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2,2), &wsa_data);
  
  addrinfo hints = {0};
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags    = AI_PASSIVE;
  
  addrinfo *address_info = NULL;
  getaddrinfo(NULL, DEFAULT_PORT, &hints, &address_info);

  SOCKET server_socket = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);

  bind(server_socket, address_info->ai_addr, (s32)address_info->ai_addrlen);

  freeaddrinfo(address_info);

  listen(server_socket, SOMAXCONN);

  SOCKET client_sockets[MAX_SHIPS] = {0};
  u8 client_index = 0;

  while(client_index < MAX_SHIPS) {
    client_sockets[client_index] = accept(server_socket, NULL, NULL);

    if(client_sockets[client_index] != INVALID_SOCKET) {
      #define ID_BUFFER_SIZE 1
      char id_buffer[ID_BUFFER_SIZE] = {client_index};
      send(client_sockets[client_index], id_buffer, ID_BUFFER_SIZE, 0);
      client_index++;
    }
  }

  DWORD non_blocking = 1;
  ioctlsocket(server_socket, FIONBIO, &non_blocking);

  while(true) {
    for(u8 i = 0; i < MAX_SHIPS; i++) {
      char receive_buffer[MESSAGE_BUFFER_SIZE];
      s32 recv_result = recv(client_sockets[i], receive_buffer, MESSAGE_BUFFER_SIZE, 0);
      if(recv_result <= 0) continue;

      for(u8 j = 0; j < MAX_SHIPS; j++) {
        u8 player_id_sender = receive_buffer[0];
        if(player_id_sender == j) continue;

        s32 send_result = send(client_sockets[j], receive_buffer, MESSAGE_BUFFER_SIZE, 0);
        if(send_result == SOCKET_ERROR) {
          printf("send failed with error: %d\n", WSAGetLastError());
          return 1;
        }
      }
    }
  }

  return 0;
}