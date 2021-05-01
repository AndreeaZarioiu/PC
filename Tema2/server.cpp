#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "helper.h"
using namespace std;

bool convert_msg(udp_message *msg1, tcp_message *msg2) {
  if (msg1->data_type > 3 || msg1->data_type < 0) {
    printf("Unknown data_type.\n");
    return FALSE;
  }
  bool sign = false;
  /*received INT*/
  if (msg1->data_type == 0) {
    strcpy(msg2->data_type, "INT");
    if (msg1->content[0]) {
      sign = true;
    }
    long long aux = ntohl(*(uint32_t *)(msg1->content + 1));
    if (sign) aux *= -1;
    sprintf(msg2->content, "%lld", aux);
    return TRUE;
  }
  /*received SHORT REAL*/
  if (msg1->data_type == 1) {
    strcpy(msg2->data_type, "SHORT_REAL");
    double aux = ntohs(*(uint16_t *)msg1->content);
    double aux2 = aux / 100;
    sprintf(msg2->content, "%0.2f", aux2);
    return TRUE;
  }
  /*received FLOAT*/
  if (msg1->data_type == 2) {
    strcpy(msg2->data_type, "FLOAT");
    if (msg1->content[0]) {
      sign = true;
    }
    double aux = ntohl(*(uint32_t *)(msg1->content + 1));
    double aux2 = aux / pow(10, msg1->content[5]);
    if (sign) aux2 *= -1;
    sprintf(msg2->content, "%lf", aux2);
    return TRUE;
  }
  /*received STRING*/
  if (msg1->data_type == 3) {
    strcpy(msg2->data_type, "STRING");
    strcpy(msg2->content, msg1->content);
    return TRUE;
  }
  return FALSE;
}

void usage(char *file) {
  fprintf(stderr, "Usage: %s server_port\n", file);
  exit(0);
}

int main(int argc, char *argv[]) {
  int tcp_socket, portno, udp_socket, new_sock;
  char buffer[BUFLEN];
  struct sockaddr_in tcp_serv_addr, tcp_cli_addr, udp_serv_addr;
  socklen_t clilen;
  udp_message *received;
  tcp_message to_send;
  from_subscriber *msg;
  int active_subscribers = 0;

  unordered_map<string, vector<int>> domains;
  unordered_map<int, client> subscribers;
  unordered_map<string, vector<string>> toStore;

  unordered_map<string, bool> unsubscribed;
  unordered_map<string, vector<tcp_message>> backUp;
  unordered_map<string, vector<string>> ifEverReturn;
  fd_set read_fds;
  fd_set tmp_fds;
  int fdmax;

  // if number or arguments is wrong
  if (argc != 2) {
    usage(argv[0]);
  }

  // clear socket
  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  /* open tcp socket */
  tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (tcp_socket < 0) perror("Socket tcp failed!");

  /* open udp socket*/
  udp_socket = socket(PF_INET, SOCK_DGRAM, 0);
  if (udp_socket < 0) perror("Socket udp failed!");

  // port number
  portno = atoi(argv[1]);
  if (portno == 0) perror("Failed atoi portno");

  // for stdin
  FD_SET(tcp_socket, &read_fds);
  fdmax = tcp_socket;
  FD_SET(0, &read_fds);

  // complete tcp socket
  memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
  tcp_serv_addr.sin_family = AF_INET;
  tcp_serv_addr.sin_port = htons(portno);
  tcp_serv_addr.sin_addr.s_addr = INADDR_ANY;

  // complete udp_socket
  udp_serv_addr.sin_family = AF_INET;
  udp_serv_addr.sin_port = htons(portno);
  udp_serv_addr.sin_addr.s_addr = INADDR_ANY;

  // bind with given port for tcp
  int ret = bind(tcp_socket, (struct sockaddr *)&tcp_serv_addr,
                 sizeof(struct sockaddr));
  if (ret < 0) perror("Bind failed!");

  // bind for udp
  ret = bind(udp_socket, (struct sockaddr *)&udp_serv_addr,
             sizeof(struct sockaddr));
  if (ret < 0) perror("Bind failed!");

  ret = listen(tcp_socket, 3);
  if (ret < 0) perror("listen failed!");

  FD_SET(tcp_socket, &read_fds);
  FD_SET(udp_socket, &read_fds);
  fdmax = max(tcp_socket, udp_socket);
  client c1;

  bool need_id = false;

  while (TRUE) {
    tmp_fds = read_fds;

    /*waiting for some activity*/
    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    if (ret < 0) perror("Select error");

   
    for (int j = 0; j <= fdmax; j++) {
      if (FD_ISSET(j, &tmp_fds)) {
         // received a tcp connection request
        if (j == tcp_socket) {
          clilen = sizeof(tcp_cli_addr);
          new_sock = accept(j, (struct sockaddr *)&tcp_cli_addr, &clilen);
          if (new_sock < 0) perror("Accept failed!");

          int ok = 1;
          setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, &ok, sizeof(int));
          FD_SET(new_sock, &read_fds);
          if (new_sock > fdmax) fdmax = new_sock;

          // new client

          need_id = true;
          c1.fd = new_sock;
          c1.port = ntohs(tcp_cli_addr.sin_port);
          c1.ip = inet_ntoa(tcp_cli_addr.sin_addr);
          // add new socket to map

          subscribers[new_sock] = c1;
          active_subscribers++;

          continue;
        }
        if (j == udp_socket) {
          memset(buffer, 0, BUFLEN);
          socklen_t udp_client_len = sizeof(sockaddr);
          int udp_client =
              recvfrom(udp_socket, buffer, BUFLEN, 0,
                       (struct sockaddr *)&udp_serv_addr, &udp_client_len);
          strcpy(to_send.ip, inet_ntoa(udp_serv_addr.sin_addr));
          to_send.port = ntohs(udp_serv_addr.sin_port);
          received = (udp_message *)buffer;
          strncpy(to_send.topic, received->topic, 50);
          if (!convert_msg(received, &to_send)) {
            printf("Wrong udp_message\n");
            continue;
          }
          
          for (auto y : domains[received->topic]) {
            send(y, (char *)&to_send, sizeof(tcp_message), 0);
          }
          for (auto y : toStore[received->topic]) {
            if (unsubscribed[y]) {
              backUp[y].push_back(to_send);
            }
          }

          continue;
        }
        // the exit command
        if (j == STDIN_FILENO) {
          memset(buffer, 0, BUFLEN);
          fgets(buffer, BUFLEN - 1, stdin);
          if (strcmp(buffer, "exit\n") == 0) {
            for (auto x : subscribers) {
              send(x.first, buffer, strlen(buffer), 0);
              close(x.first);
            }

            FD_CLR(j, &read_fds);
            FD_CLR(j, &tmp_fds);
            FD_CLR(tcp_socket, &read_fds);
            FD_CLR(tcp_socket, &tmp_fds);
            close(tcp_socket);
            close(udp_socket);
            return 0;
          } else {
            printf("Unknown command received from stdin.\n");
          }
          continue;
        } else {
          // data received from clients
          memset(buffer, 0, BUFLEN);
          int act = recv(j, buffer, sizeof(buffer), 0);
          if (act < 0) perror("Received failed!");

          if (act == 0) {  // client unsubscribed from server

            printf("Client %s disconnected\n", subscribers[j].id);

            for (auto x : domains) {
              domains[x.first].erase(remove( domains[x.first].begin(),  domains[x.first].end(), j),
                                domains[x.first].end());
            }

            string aux(subscribers[j].id);
            unsubscribed[aux] = TRUE;
            subscribers.erase(j);
            close(j);

            FD_CLR(j, &read_fds);
          } else if (need_id) {
            bool no = false;
            strcpy(subscribers[j].id, buffer);
            for (auto x : subscribers) {
              if (strcmp((x.second).id, subscribers[j].id) == 0 &&
                  x.first != j) {
                memset(buffer, 0, BUFLEN);
                strcpy(buffer, "wrong_id");
                send(j, buffer, strlen(buffer), 0);
                no = true;
              }
            }
            if (no) {
              subscribers.erase(j);
              continue;
            }
            printf("New client %s connected from %s : %d\n", subscribers[j].id,
                   subscribers[j].ip, subscribers[j].port);
            need_id = false;
            string aux(subscribers[j].id);
            if (unsubscribed.find(aux) == unsubscribed.end() ||
                unsubscribed[aux] == FALSE)
              continue;
            unsubscribed[aux] = FALSE;
            // send what stored
            for (auto y : backUp[aux]) {
              send(j, (char *)&y, sizeof(tcp_message), 0);
            }
            // again updated
            if (ifEverReturn.find(aux) != ifEverReturn.end()) {
              for (auto y : ifEverReturn[aux]) {
                domains[y].push_back(j);
              }
            }
            // nothing to send to this client
            backUp[aux].clear();
            

          } else {
            msg = (from_subscriber *)buffer;
            if (msg->act == 's') {
              string s(msg->topic);
              // if not already subscribed
              if ((domains.find(s) != domains.end() &&
                   find(domains[s].begin(), domains[s].end(), j) ==
                       domains[s].end()) ||
                  domains.find(s) == domains.end()) {
                string s1(subscribers[j].id);
                ifEverReturn[s1].push_back(s);
                domains[s].push_back(j);
                if (msg->sf == 1) {
                  if ((toStore.find(s) != toStore.end() &&
                       find(toStore[s].begin(), toStore[s].end(), s1) ==
                           toStore[s].end()) ||
                      (toStore.find(s) == toStore.end()))
                    toStore[s].push_back(s1);
                }
              }

            } else if (msg->act == 'u') {
              string s(msg->topic);
              string aux(subscribers[j].id);
              if (domains.find(s) == domains.end()) continue;
              // don't send about this topic
              domains[s].erase(remove(domains[s].begin(), domains[s].end(), j),
                               domains[s].end());
              ifEverReturn[aux].erase(
                  remove(ifEverReturn[aux].begin(), ifEverReturn[aux].end(), s),
                  ifEverReturn[aux].end());
              // don't store data about this topic if sf 1
              if (toStore.find(s) == toStore.end()) continue;

              toStore[s].erase(
                  remove(toStore[s].begin(), toStore[s].end(), aux),
                  toStore[s].end());
            } else {
              printf("Unknown command received from tcp client.\n");
            }
          }
        }
      }
    }
  }

  close(tcp_socket);
  close(udp_socket);

  return 0;
}