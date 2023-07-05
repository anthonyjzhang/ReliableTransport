#pragma once

#include "Util.h"
#include "Reliable.h"
#include "ReliableImpl.h"

#define SS 0
#define CA 1
#define MIN_RTO 0.3

void updateCWND(Reliable *reli, ReliableImpl *reliImpl, bool acked, bool loss, bool fast);
double updateRTO(Reliable *reli, ReliableImpl *reliImpl, double timestamp);
