#include "ReliableImpl.h"
#include "Congestion.h"

// check if index is in [head, tail)
static bool checkInWrapRange(uint32_t head, uint32_t tail, uint32_t index)
{
    if (head <= tail && (index < head || tail <= index))
        return false;
    if (tail < head && (tail <= index && index < head))
        return false;
    return true;
}

static uint32_t wrapdist(uint32_t head, uint32_t tail)
{
    return (head <= tail) ? tail - head : (UINT32_MAX - head) + tail + 1;
}

static bool checkAcked(uint32_t seqNum, uint32_t payloadlen, uint32_t ackNum)
{
    uint32_t dist = wrapdist(seqNum, ackNum);
    return (dist <= INT32_MAX && dist >= payloadlen);
}

// 'seqNum' indicates the initail sequence number in the SYN segment.
// 'srvSeqNum' indicates the initial sequence number in the SYNACK segment.
ReliableImpl *reliImplCreate(Reliable *_reli, uint32_t _seqNum, uint32_t _srvSeqNum)
{
    ReliableImpl *reliImpl = (ReliableImpl *)malloc(sizeof(ReliableImpl));
    reliImpl->reli = _reli;
    reliImpl->lastByteSent = _seqNum;
    reliImpl->nextByteExpected = _srvSeqNum + 1; // nextByteExpected remains unchanged in this lab

    queueInit(&reliImpl->swnd, MAX_BDP / PAYLOAD_SIZE); // a queue to store sent segments
    reliImpl->lastByteAcked = _seqNum;
    reliImpl->lastAckNum = reliImpl->lastByteAcked + 1;

    reliImpl->status = SS;
    reliImpl->ssthresh = 20000;
    reliImpl->rto = MIN_RTO;
    reliImpl->srtt = -1;
    reliImpl->rttvar = -1;
    reliImpl->FRCount = 0; // Count for fast retransmission

    return reliImpl;
}

// reliImplClose: Destructor, free the memory for maintaining states.
void reliImplClose(ReliableImpl *reliImpl)
{
    if (reliImpl == NULL)
        return;

    while (reliImpl->swnd.count > 0)
    {
        Context *ctx = queueFront(&reliImpl->swnd);
        Free(ctx);
        queuePop(&reliImpl->swnd);
    }
    queueClear(&reliImpl->swnd);

    Free(reliImpl);
}

// 16-bit Internet checksum (refer to RFC 1071 for calculation
uint16_t reliImplChecksum(const char *buf, ssize_t len)
{
    uint32_t sum = 0;
    for (ssize_t i = 0; i < len - 1; i += 2)
        sum += *(uint16_t *)(buf + i);
    if (len & 1)
    {
        uint16_t tmp = 0;
        *((char *)(&tmp)) = buf[len - 1];
        sum += tmp;
    }
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}

// 'buf' is an array of bytes of the received segment, including the segment header.
// 'len' is the length of 'buf'.
uint32_t reliImplRecvAck(ReliableImpl *reliImpl, const char *buf, uint16_t len)
{
    SegmentHdr *seg = (SegmentHdr *)buf;
    uint32_t ackNum = ntohl(seg->ackNum);
    Context *ctx = queueFront(&reliImpl->swnd);

    if (!checkInWrapRange(reliImpl->lastAckNum + 1, reliImpl->lastByteSent + 2, ackNum))
        return 0;
    if (ackNum == reliImpl->lastAckNum)
    {
        reliImpl->FRCount = reliImpl->FRCount + 1;
        if (reliImpl->FRCount == 3)
        {
            reliImplFastRetransmission(ctx);
            return 0;
        }
    }

    reliImpl->FRCount = 0;
    uint32_t fbytesReduce = wrapdist(reliImpl->lastAckNum, ackNum);
    reliImpl->lastByteAcked = ackNum - 1;
    reliImpl->lastAckNum = ackNum;

    if (ctx != NULL && !ctx->resendFlag)
    {
        updateRTO(ctx->reliImpl->reli, ctx->reliImpl, ctx->currentTime);
    }
    while (reliImpl->swnd.count > 0)
    {
        Context *ctx = queueFront(&reliImpl->swnd);
        if (!checkAcked(ctx->seqNum, ctx->payloadlen, reliImpl->lastAckNum))
            break;
        timerCancel(ctx->timer);
        Free(ctx->buf);
        Free(ctx);
        updateCWND(ctx->reliImpl->reli, ctx->reliImpl, 1, 0, 0);
        queuePop(&reliImpl->swnd);
    }
    reliUpdateRWND(reliImpl->reli, ntohl(seg->rwnd));

    return fbytesReduce;
}

// 'payload' is an array of char bytes.
// 'payloadlen' is the length of payload.
// 'isFin'=True means a FIN segment should be sent out.
uint32_t reliImplSendData(ReliableImpl *reliImpl, char *payload, uint16_t payloadlen, bool isFin)
{
    uint32_t seqNum = reliImpl->lastByteSent + 1;
    uint16_t buflen = sizeof(SegmentHdr) + payloadlen;
    
    char *buf = (char *)malloc(buflen * sizeof(char));

    SegmentHdr *seg = (SegmentHdr *)buf;
    seg->seqNum = htonl(seqNum);
    seg->ackNum = htonl(reliImpl->nextByteExpected);
    seg->rwnd = 0;
    seg->ack = seg->syn = 0;
    seg->fin = isFin;
    seg->checksum = 0;
    memcpy(buf + sizeof(SegmentHdr), payload, payloadlen);
    seg->checksum = reliImplChecksum(buf, buflen);

    payloadlen = MAX(payloadlen, (uint16_t)1);

    Context *ctx = (Context *)malloc(sizeof(Context));

    *ctx = (Context){seqNum, buf, buflen, payloadlen, reliImpl->rto, NULL, reliImpl, 0, 0};
    ctx->timer = reliSetTimer(reliImpl->reli, reliImpl->rto, reliImplRetransmission, ctx);
    ctx->resendFlag = 0;
    ctx->currentTime = get_current_time();
    queuePush(&reliImpl->swnd, ctx);
    reliImpl->lastByteSent += (uint32_t)payloadlen;

    reliSendto(reliImpl->reli, buf, buflen);
    return payloadlen;
}

void *reliImplRetransmission(void *args)
{
    Context *ctx = (Context *)args;
    if (checkAcked(ctx->seqNum, ctx->payloadlen, ctx->reliImpl->lastAckNum))
        return NULL;
    if (ctx->rto >= MAX_RTO)
        ErrorHandler("Ack time out.");

    if (ctx->seqNum == ctx->reliImpl->lastAckNum)
    {
        updateCWND(ctx->reliImpl->reli, ctx->reliImpl, 0, 1, 0);
    }

    ctx->rto = MIN(MAX_RTO, ctx->rto * 2);
    ctx->timer = reliSetTimer(ctx->reliImpl->reli, ctx->rto, reliImplRetransmission, ctx);
    ctx->resendFlag = 1;
    reliSendto(ctx->reliImpl->reli, ctx->buf, ctx->buflen);
    return NULL;
}

void *reliImplFastRetransmission(void *args)
{
    Context *ctx = (Context *)args;
    timerCancel(ctx->timer);
    ctx->rto = MIN(MAX_RTO, ctx->rto * 2);
    ctx->timer = reliSetTimer(ctx->reliImpl->reli, ctx->rto, reliImplRetransmission, ctx);
    ctx->resendFlag = 1;
    reliSendto(ctx->reliImpl->reli, ctx->buf, ctx->buflen);
    return NULL;
}