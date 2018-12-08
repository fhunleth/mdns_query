#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef WIN32
#include <ws2tcpip.h>
#include <errno.h>
#include <stdarg.h>
static void err(int status, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    int err = errno;
    fprintf(stderr, "mdns_query: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, ": %s\n", strerror(err));
    exit(status);

    va_end(ap);
}
#else
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


static const uint8_t query[] = {
    0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0x8, 0x73, 0x72,
    0x68, 0x2d, 0x30, 0x30, 0x30, 0x30, 0x05,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0, 0, 0x01, 0, 0x01
};


int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in destaddr;
    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(5353);
     inet_aton("224.0.0.251", &destaddr.sin_addr);
//destaddr.sin_addr.s_addr = htonl(0xe00000fb); // 224.0.0.251

    struct sockaddr_in ouraddr;
    memset(&ouraddr, 0, sizeof(ouraddr));
    ouraddr.sin_family = AF_INET;
    ouraddr.sin_port = htons(5353);
    ouraddr.sin_addr.s_addr = htonl(INADDR_ANY);

#ifdef WIN32
    BOOL enable = TRUE;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*) &enable, sizeof(enable)) == SOCKET_ERROR)
        err(EXIT_FAILURE, "setsockopt(SO_REUSEADDR)");

#else
    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        err(EXIT_FAILURE, "setsockopt(SO_REUSEADDR)");
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0)
        err(EXIT_FAILURE, "setsockopt(SO_REUSEPORT)");
#endif
    if (bind(s, (struct sockaddr *) &ouraddr, sizeof(ouraddr)) < 0)
        err(EXIT_FAILURE, "bind");

    if (sendto(s, query, sizeof(query), 0, (struct sockaddr *) &destaddr, sizeof(destaddr)) < 0)
        err(EXIT_FAILURE, "sendto");

    for (;;) {
        struct sockaddr_storage respaddr;
        memset(&respaddr, 0, sizeof(respaddr));
        socklen_t len;
        uint8_t buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *) &respaddr, &len) < 0)
            err(EXIT_FAILURE, "recvfrom");

        if (buffer[2] == 0x84 && buffer[13] == 's' && buffer[14] == 'r' && buffer[15] == 'h') {
            struct sockaddr_in *respaddr4 = (struct sockaddr_in *) &respaddr;
            printf("%s\n", inet_ntoa(respaddr4->sin_addr));

            printf("%d.%d.%d.%d\n", buffer[38], buffer[39], buffer[40], buffer[41]);
            break;
        }
    }
    close(s);
}
