#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NBACKLOG 100
#define NPROCESS 10

int
respond(int clientfd)
{
  char buf[256];
  ssize_t len, off;

  while ((len = read(clientfd, buf, sizeof buf)) != -1) {
    off = 0;
    while (len > off) {
      off += write(clientfd, buf + off, len - off);
    }
  }

  return 0;
}

int
serve(int port, char *root_path)
{
  int serverfd;
  struct sockaddr_in addr;
  size_t addrlen;
  int i;
  pid_t pid;

  serverfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* host */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  addrlen = sizeof(addr);

  if (bind(serverfd, (struct sockaddr *)&addr, addrlen) == -1) {
    perror("bind");
    return -1;
  }

  if (listen(serverfd, NBACKLOG) == -1) {
    perror("listen");
    return -1;
  }

  for (i = 0; i < NPROCESS; ++i) {
    if ((pid = fork()) == 0) {
      int client;

      while (1) {
        if ((client = accept(serverfd, NULL, 0)) == -1) {
          perror("accept");
          return -1;
        }

        if (respond(client) == -1) {
          fprintf(stderr, "server error");
        }

        close(client);
      }
      return 0;
    }
  }

  for (i = 0; i < NPROCESS; ++i) {
    wait(NULL);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  int port;

  port = atoi(argv[1]);

  serve(port, argv[2]);

  return 0;
}
