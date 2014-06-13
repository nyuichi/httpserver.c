#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NBACKLOG 100
#define NPROCESS 10

void
getdate(char *buf, size_t len)
{
  time_t now;

  now = time(0);
  strftime(buf, len, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&now));
}

int
respond(FILE *fp)
{
  char buf[2048];

  do {
    fgets(buf, sizeof buf, fp);
  } while (strlen(buf) > 2);

  getdate(buf, sizeof buf);

  fprintf(fp, "HTTP/1.1 501 Not Implemented\t\n");
  fprintf(fp, "Date: %s\r\n", buf);
  fprintf(fp, "\r\n");

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
      int clientfd;
      FILE *fp;

      while (1) {
        if ((clientfd = accept(serverfd, NULL, 0)) == -1) {
          perror("accept");
          return -1;
        }

        if ((fp = fdopen(clientfd, "r+")) != NULL) {
          if (respond(fp) == -1) {
            fprintf(stderr, "server error");
          }
          fclose(fp);
        }
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
