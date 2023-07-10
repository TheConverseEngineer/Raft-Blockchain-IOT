#ifndef ENTRY_H
#define ENTRY_H

#include "mac.h"

struct Entry {
    int term;
    long long time;
    MACAddress storageLocation;
};

#endif