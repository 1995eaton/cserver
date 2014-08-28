#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#define PORT "80"
#define HOSTNAME NULL
#define MAX_CONNECTIONS 10

const char header[] = "HTTP/1.1 200 OK\nContent-Type: text/html\r\n\r\n";

char *get_ip(struct sockaddr_in* si) {
  unsigned char *ipa = (unsigned char*)&si->sin_addr.s_addr;
  char *ip = malloc(16 * sizeof(char));
  sprintf(ip, "%d.%d.%d.%d", ipa[0], ipa[1], ipa[2], ipa[3]);
  return ip;
}

int create_server(void) {
  static const int yes = 1;
  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (HOSTNAME == NULL) {
    // use local network assigned IP instead of 127.0.0.1
    hints.ai_flags = AI_PASSIVE;
  }

  assert(getaddrinfo(HOSTNAME, PORT, &hints, &res) == 0);

  int sockfd; // local socket file descriptor

  // filter through the linked list of results until a connectable socket is found
  while (res != NULL) {
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd != -1) {
      printf("opened socket connection\n");
      break;
    }
    close(sockfd);
    res = res->ai_next;
  }

  assert(sockfd != -1);
  // attempt to close an existing socket if it conflicts with sockfd
  assert(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == 0);
  // bind to socket before listening for connections
  assert(bind(sockfd, res->ai_addr, res->ai_addrlen) == 0);
  assert(listen(sockfd, MAX_CONNECTIONS) == 0);

  freeaddrinfo(res);
  return sockfd;
}

typedef struct peer_t {
  int fd;
  struct sockaddr_storage addr;
  socklen_t addrlen;
  char *ip;
} peer_t;

peer_t *create_peer(int sockfd) {
  peer_t *peer = (peer_t*) malloc(sizeof(peer_t));
  peer->addrlen = sizeof(peer->addr);
  peer->fd = accept(sockfd, (struct sockaddr*)&peer->addr, &peer->addrlen);
  assert(peer->fd != -1);
  peer->ip = get_ip((struct sockaddr_in*)&peer->addr);
  return peer;
}

void *handle(void *peerp) {
  peer_t *peer = peerp;
  char buf[1024];
  memset(buf, 0, sizeof(buf) * sizeof(char));
  printf("%s connected\n", peer->ip);
  while (read(peer->fd, buf, sizeof(buf) * sizeof(char)) > 0) {
    if (buf[0] != -1) {
      puts(buf);
    }
    memset(buf, 0, sizeof(buf) * sizeof(char));
    // TODO parse headers/look for HTTP header termination string
    break;
  }
  char message[] = "<h1>Hello</h1>";
  char *body = (char*) malloc(sizeof(header) + sizeof(message));
  strncat(body, header, sizeof(header));
  strncat(body, message, sizeof(message));
  send(peer->fd, body, strlen(body), 0);
  close(peer->fd);
  free(peer->ip);
  free(peer);
  free(body);
  pthread_exit(0);
}

int main(void) {
  int sockfd = create_server();
  pthread_t thread;
  while (1) {
    peer_t *peer = create_peer(sockfd);
    pthread_create(&thread, NULL, handle, peer);
  }
  close(sockfd);
  return 0;
}
