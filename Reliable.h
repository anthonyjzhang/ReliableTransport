#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <signal.h>
#include "Util.h"

typedef struct Reliable Reliable;
#include "ReliableImpl.h"

#define SYNSENT 0
#define CONNECTED 1
#define FINWAIT 2
#define CLOSED 3

typedef struct Payload
{
    char *buf;
    uint16_t len;
    bool fin;
} Payload;

Payload *payloadCreate(uint16_t size, bool fin);
void payloadClose(Payload *payload);

struct Reliable
{
    int skt;
    struct sockaddr_in srvaddr;
    socklen_t srvlen;

    short status;
    uint32_t bytesInFlight, rwnd, cwnd;
    SafeQueue buffer;
    Heap timerHeap;

    char *seg_str;
    ReliableImpl *reliImpl;
    pthread_t thHandler;
};

Reliable *reliCreate(unsigned hport);
void reliClose(Reliable *reli);
int reliConnect(Reliable *reli, const char *ip, unsigned rport, bool nflag, uint32_t n);
void *reliGetPayload(Reliable *reli);           // return NULL if queue is empty
int reliSend(Reliable *reli, Payload *payload); // block if queue is full

ssize_t reliRecvfrom(Reliable *reli, char *seg_str, size_t size);

ssize_t reliSendto(Reliable *reli, const char *seg_str, const size_t len);

// updateRWND: Update the receive window size.
//'rwnd' is the bytes of the receive window.
uint32_t reliUpdateRWND(Reliable *reli, uint32_t _rwnd);

Timer *reliSetTimer(Reliable *reli, double timesec, void *(*callback)(void *), void *args);