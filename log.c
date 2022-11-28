#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

#include "log.h"
#include "fs.h"
#include "buf.h"

static void recover(uint dev);
static void readhead(uint dev);
static void writehead(uint dev);
static void install_trans(uint dev);

struct logmeta logmeta[MAXDEVICE];

void loginit(void) {
    memset(&logmeta, 0, (sizeof logmeta/sizeof logmeta[0]) * MAXDEVICE);
}

void initlog(uint dev) {
    struct logmeta *l;
    struct superblock sb;
    
    if(dev > MAXDEVICE) {
        printf("device %d is not a valid device\n", dev);
        exit(1);
    }
    
    memset(&sb, 0, sizeof sb);
    readsb(dev, &sb);
    
    l = &logmeta[dev];
    
    l->start = sb.size - sb.nlog;
    l->size = sb.nlog;
    l->dev = dev;
    
    recover(dev);
    
}

static void readhead(uint dev) {
    struct buf *b;
    struct logmeta *l;
    struct logheader *lh;
    int i;
    
    l = &logmeta[dev];
    
    b = bread(l->dev, l->start);
    lh = (struct logheader *) b->buffer;
    
    l->lh.n = lh->n;
    
    for(i = 0; i < l->lh.n; i++)
        l->lh.sector[i] = lh->sector[i];
    
    brelse(b);
}

static void writehead(uint dev) {
    struct buf *b;
    struct logmeta *l;
    struct logheader *lh;
    int i;
    
    l = &logmeta[dev];
    
    b = bread(l->dev, l->start);
    lh = (struct logheader *) b->buffer;
    
    lh->n = l->lh.n;
    
    for(i = 0; i < l->lh.n; i++)
         lh->sector[i] = l->lh.sector[i];
    
    bwrite(b);
    brelse(b);
}

static void recover(uint dev) {
    readhead(dev);
    install_trans(dev);
    logmeta[dev].lh.n = 0;
    writehead(dev);
}


static void install_trans(uint dev) {
    struct logmeta *l;
    int i;
    
    struct buf *dst;
    struct buf *src;
    
    l = &logmeta[dev];
    
    for(i = 0; i < l->lh.n; i++) {
        
        src = bread(l->dev, l->start + i + 1);
        dst = bread(l->dev, l->lh.sector[i]);
        
        memmove(dst, src, RAMSIZE);
        bwrite(dst);
        brelse(dst);
        brelse(src);
        
    }
    
   
}

void begin(void) {
    
}

void commit(void) {
    int i;
    struct logmeta *l;
    
    
   
    for(i = 0; i < MAXDEVICE; i++) {
        l = &logmeta[i];
        if(l->lh.n > 0) {
            writehead(i);
            install_trans(i);
            l->lh.n = 0;
            writehead(i);
        }
    }
    
}

void log_write(struct buf *b) {
    int i;
    struct logmeta *l;
    struct buf *logbuf;
    
    l = &logmeta[b->dev];
    
    if(l->lh.n >= l->size) {
        printf("Transaction size too big\n");
        exit(1);
    }
    
    for(i = 0; i < l->lh.n; i++) {
        if(l->lh.sector[i] == b->sector) {
            break;
        }
    }
    
    l->lh.sector[i] = b->sector;
    
    logbuf = bread(b->dev, l->start+i+1);
    memmove(logbuf->buffer, b->buffer, RAMSIZE);
    bwrite(logbuf);
    brelse(logbuf);
    
    if(i == l->lh.n)
        l->lh.n++;
        
    b->flags |= B_DIRTY;
}
