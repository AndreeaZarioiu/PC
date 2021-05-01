#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vector"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helper.h"

void usage(char *file) {
  fprintf(stderr, "Usage: %s server_port\n", file);
  exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, n, ret;
  struct sockaddr_in serv_addr;
  char buffer[BUFLEN];
  tcp_message *received;
  fd_set read_fds;
  fd_set tmp_fds;
  from_subscriber msg;
  int fdmax;

  FD_ZERO(&tmp_fds);
  FD_ZERO(&read_fds);

  if (argc != 4) {
    usage(argv[0]);
  }

  /* open tcp socket */
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) perror("Socket tcp failed!");

  FD_SET(sockfd, &read_fds);
  fdmax = sockfd;
  FD_SET(0, &read_fds);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[3]));
  ret = inet_aton(argv[2], &serv_addr.sin_addr);
  if (ret == 0) perror("inet_aton failed!");
  if (strlen(argv[1]) > 10) {
    printf("Size of id too big.");
    return 0;
  }

  ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (ret < 0) perror("connect failed!");
  n = send(sockfd, argv[1], strlen(argv[1]), 0);
  if (n < 0) perror("send id failed!");

  while (TRUE) {
    tmp_fds = read_fds;
    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    if (ret < 0) perror("select failed!");

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == sockfd) {
          memset(buffer, 0, BUFLEN);
          n = recv(i, buffer, sizeof(tcp_message), 0);
          if (n < 0) perror("recv failed!");

          // receiving from server
          if (strcmp(buffer, "exit\n") == 0) {
            FD_CLR(i, &read_fds);
            FD_CLR(i, &tmp_fds);
            FD_CLR(sockfd, &read_fds);
            FD_CLR(sockfd, &tmp_fds);
            close(sockfd);
            return 0;
          } else if (strcmp(buffer, "wrong_id") == 0) {
            printf("Id already in use.\n");
            FD_CLR(i, &read_fds);
            FD_CLR(i, &tmp_fds);
            FD_CLR(sockfd, &read_fds);
            FD_CLR(sockfd, &tmp_fds);
            close(sockfd);
            return 0;
          }
          received = (tcp_message *)buffer;
          printf("%s:%d - %s - %s - %s \n", received->ip, received->port,
                 received->topic, received->data_type, received->content);

          continue;
        }

        if (i == STDIN_FILENO) {
          memset(buffer, 0, BUFLEN);
          fgets(buffer, BUFLEN - 1, stdin);

          if (strcmp(buffer, "exit\n") == 0) {
            FD_CLR(i, &read_fds);
            FD_CLR(i, &tmp_fds);
            FD_CLR(sockfd, &read_fds);
            FD_CLR(sockfd, &tmp_fds);
            close(sockfd);
            return 0;
          }
          char act[12];
          char topic[50];
          int sf = -1;
          memset(act, 0, 12);
          memset(topic, 0, 50);

          if (strncmp(buffer, "subscribe ", 10) == 0) {
            sscanf(buffer, "%s %s %d", act, topic, &sf);
            if (sf != 0 && sf != 1) {
              printf("Sf must be 0 or 1. Please retype.\n");
              continue;
            }
            msg.act = 's';
            msg.sf = sf;
            strcpy(msg.topic, topic);
            strcpy(act, "subscribed");
          } else if (strncmp(buffer, "unsubscribe ", 12) == 0) {
            sscanf(buffer, "%s %s", act, topic);
            msg.act = 'u';
            strcpy(msg.topic, topic);
            msg.sf = -1;
            strcpy(act, "unsubscribed");

          } else {
            printf("Unknown command received from stdin\n");
            continue;
          }

          // send to server
          n = send(sockfd, (char *)&msg, sizeof(from_subscriber), 0);
          if (n < 0) perror("send failed!");
          printf("%s %s\n", act, msg.topic);
        }
      }
    }
  }

  close(sockfd);
}
