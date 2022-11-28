#pragma once
#include "main.h"


struct RAMBUFFER {
    unsigned char sector[RAMSIZE];
};

struct device {
    uint deviceID;
    uint size;
    struct RAMBUFFER *DB;
};

void RAMinit(void);
void wRAM(uint dev, uint sector, const unsigned char *data);
void rRAM(uint dev, uint sector, unsigned char *data);
