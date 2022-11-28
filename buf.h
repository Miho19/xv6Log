#pragma once
#include "main.h"


struct buf {
    struct buf *next;
    struct buf *prev;
    unsigned char buffer[RAMSIZE];
    uint sector;
    uint dev;
    int flags;
};





struct buf * bread(uint dev, uint sector);
void brelse(struct buf *b);
void bwrite(struct buf *b);
void brelse(struct buf *b);
void printbuf(int index, struct buf *b);
void usbrw(struct buf *b);
