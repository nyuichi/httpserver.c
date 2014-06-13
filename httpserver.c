#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NBACKLOG 100
#define NPROCESS 10

int
serve(int port)
{
  int sock;
  struct sockaddr_in addr;
  size_t addrlen;
  int i;
  pid_t pid;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* host */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  addrlen = sizeof(addr);

  if (bind(sock, (struct sockaddr *)&addr, addrlen) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(sock, NBACKLOG) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < NPROCESS; ++i) {
    if ((pid = fork()) == 0) {
      int tcp;
      char buf[256];
      ssize_t len, off;

      while (1) {
        if ((tcp = accept(sock, NULL, 0)) == -1) {
          perror("accept");
          exit(EXIT_FAILURE);
        }

        while ((len = read(tcp, buf, sizeof buf)) != -1) {
          off = 0;
          while (len > off) {
            off += write(tcp, buf + off, len - off);
          }
        }

        close(tcp);
      }
      exit(0);
    }
  }

  for (i = 0; i < NPROCESS; ++i) {
    wait(NULL);
  }
}

int
main(int argc, char *argv[])
{
  int port;

  port = atoi(argv[1]);

  serve(port);
}
