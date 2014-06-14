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

void
rGET(FILE *fp, char *path)
{
  char buf[2048];
  FILE *file;
  size_t len, size;

  printf("in rGET"); fflush(stdout);

  getdate(buf, sizeof buf);

  fprintf(fp, "HTTP/1.1 200 OK\r\n");
  fprintf(fp, "Date: %s\r\n", buf);
  fprintf(fp, "\r\n");

  file = fopen(path, "rb");

  do {
    len = fread(buf, 1, sizeof buf, file);
    if (ferror(file)) {
      break;
    }
    size = 0;
    while (size < len) {
      size += fwrite(buf + size, 1, len - size, fp);
    }
  } while (! feof(file));
  fflush(fp);

  fclose(file);
}

void
rHEAD(FILE *fp)
{
  char date[2048];

  getdate(date, sizeof date);

  fprintf(fp, "HTTP/1.1 200 OK\r\n");
  fprintf(fp, "Date: %s\r\n", date);
  fprintf(fp, "\r\n");
}

void
r501(FILE *fp)
{
  char date[2048];

  getdate(date, sizeof date);

  fprintf(fp, "HTTP/1.1 501 Not Implemented\t\n");
  fprintf(fp, "Date: %s\r\n", date);
  fprintf(fp, "\r\n");
}

int
respond(FILE *fp, char *root)
{
  char method[16], path[128], buf[2048];

  fscanf(fp, "%s %s ", method, path);

  do {
    fgets(buf, sizeof buf, fp);
  } while (strlen(buf) > 2);

  if (strcmp(method, "GET") == 0) {
    strcpy(buf, root);
    strcat(buf, path);
    rGET(fp, buf);
  } else if (strcmp(method, "HEAD") == 0) {
    rHEAD(fp);
  } else {
    r501(fp);
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
      int clientfd;
      FILE *fp;

      while (1) {
        if ((clientfd = accept(serverfd, NULL, 0)) == -1) {
          perror("accept");
          return -1;
        }

        if ((fp = fdopen(clientfd, "r+")) != NULL) {
          if (respond(fp, root_path) == -1) {
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

  if (argc != 3) {
    fprintf(stderr, "expected 3 arguments, but got %d\n", argc);
    return 1;
  }

  port = atoi(argv[1]);

  serve(port, argv[2]);

  return 0;
}
