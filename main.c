#include <err.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

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

    struct sockaddr_in ouraddr;
    memset(&ouraddr, 0, sizeof(ouraddr));
    ouraddr.sin_family = AF_INET;
    ouraddr.sin_port = htons(5353);
    ouraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        err(EXIT_FAILURE, "setsockopt(SO_REUSEADDR)");
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        err(EXIT_FAILURE, "setsockopt(SO_REUSEPORT)");
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
        if (recvfrom(s, buffer, sizeof(buffer), 0, &respaddr, &len) < 0)
            err(EXIT_FAILURE, "recvfrom");

        if (buffer[2] == 0x84 && buffer[13] == 's' && buffer[14] == 'r' && buffer[15] == 'h') {
            char addrstr[64];
            struct sockaddr_in *respaddr4 = &respaddr;
            inet_ntop(AF_INET, &respaddr4->sin_addr, addrstr, sizeof(addrstr));
            fprintf(stderr, "%s\n", addrstr);

            inet_ntop(AF_INET, &buffer[38], addrstr, sizeof(addrstr));
            fprintf(stderr, "%s\n", addrstr);
            break;
        }
    }
    close(s);
}
