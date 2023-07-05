#include "Congestion.h"

#define G 0.1
#define K 4.0
#define alpha 0.125
#define beta 0.25

// updateCWND: Update reli->cwnd according to the congestion control algorithm.
// 'reli' provides an interface to access struct Reliable.
// 'reliImpl' provides an interface to access struct ReliableImpl.
// 'acked'=true when a segment is acked.
// 'loss'=true when a segment is considered lost or should be fast retransmitted.
// 'fast'=true when a segment should be fast retransmitted.
void updateCWND(Reliable *reli, ReliableImpl *reliImpl, bool acked, bool loss, bool fast)
{
    SegmentHdr *seg = (SegmentHdr *)(&reli->buffer);
    if (reliImpl->status == SS)
    {
        if (acked)
        {
            reli->cwnd = reli->cwnd + PAYLOAD_SIZE;
        }
        else if (loss)
        {
            reliImpl->ssthresh = MAX(reli->cwnd / 2, PAYLOAD_SIZE);
            reli->cwnd = reliImpl->ssthresh;
            reli->status = CA;
        }
        if (reli->cwnd > reliImpl->ssthresh)
        {
            reli->status = CA;
        }
    }
    else if (reliImpl->status == CA)
    {
        if (acked)
        {
            reli->cwnd = reli->cwnd + PAYLOAD_SIZE / (reli->cwnd / PAYLOAD_SIZE);
        }
        else if (loss)
        {
            reliImpl->ssthresh = MAX(reli->cwnd / 2, PAYLOAD_SIZE);
            reli->cwnd = reliImpl->ssthresh;
        }
    }
}

// updateRTO: Run RTT estimation and update RTO.
// 'timestamp' indicates the time when the sampled packet is sent out.
double updateRTO(Reliable *reli, ReliableImpl *reliImpl, double timestamp)
{
    double currentTime = get_current_time();
    double RTT = currentTime - timestamp;
    if (reliImpl->rttvar == 0)
    {
        reliImpl->rttvar = RTT / 2;
        reliImpl->srtt = RTT;
        reliImpl->rto = MIN(MAX_RTO, reliImpl->srtt + MAX(G, K * reliImpl->rttvar));
    }
    else
    {
        reliImpl->rttvar = (1 - beta) * reliImpl->rttvar + beta * fabs(reliImpl->srtt - RTT);
        reliImpl->srtt = (1 - alpha) * reliImpl->srtt + alpha * RTT;
        reliImpl->rto = MIN(MAX_RTO, reliImpl->srtt + MAX(G, K * reliImpl->rttvar));
    }
    if (reliImpl->rto < MIN_RTO)
    {
        reliImpl->rto = MIN_RTO;
    }
    return reliImpl->rto;
}

#undef G
#undef K
#undef alpha
#undef beta