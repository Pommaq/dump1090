//
// Created by timmy on 2021-06-06.
//

#ifndef DUMP1090_NETWORKING_HPP
#define DUMP1090_NETWORKING_HPP
#define P_FILE_GMAP "/srv/gmap.html"

#include "../Headers/Modes.hpp"
#include "../Headers/Decoding.hpp"

void modesWaitReadableClients(int timeout_ms);

int handleHTTPRequest(struct client *c);

void modesInitNet();

void modesAcceptClients();

void modesFreeClient(int fd);

void modesReadFromClients();

void modesSendSBSOutput(struct modesMessage *mm, struct aircraft *a);

void modesReadFromClient(struct client *c, char *sep,
                         int(*handler)(struct client *));

void modesSendRawOutput(struct modesMessage *mm);

void modesSendAllClients(int service, void *msg, int len);

/* Structure used to describe a networking client. */
struct client {
    int fd;         /* File descriptor. */
    int service;    /* TCP port the client is connected to. */
    char buf[MODES_CLIENT_BUF_SIZE + 1];    /* Read buffer. */
    int buflen;                         /* Amount of data on buffer. */
};

#define MODES_NET_SERVICE_RAWO 0
#define MODES_NET_SERVICE_RAWI 1
#define MODES_NET_SERVICE_HTTP 2
#define MODES_NET_SERVICE_SBS 3
#define MODES_NET_SERVICES_NUM 4

struct netservices {
    netservices(const char *ndesc, int &nsoc, int nport);

    ~netservices() = default;

    std::string descr;
    int *socket;
    int port;
};
extern netservices modesNetServices[MODES_NET_SERVICES_NUM];


#endif //DUMP1090_NETWORKING_HPP
