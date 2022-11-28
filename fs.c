#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "fs.h"
#include "buf.h"


#define IPB (RAMSIZE / sizeof(struct dinode))
#define i2b(inum) ((inum / IPB) + 2)

struct {
    struct inode inodes[NINODE];
} icache;




void readsb(uint dev, struct superblock *sb){
    struct buf *b;
    b = bread(dev, 1);
    memmove(sb, b->buffer, sizeof(*sb));
    
    brelse(b);
}

void inodeinit(void){
    memset(&icache, 0, sizeof(icache));
}

struct inode *iget(uint dev, uint inum) {
    struct inode *ip;
    struct inode *empty;
    
    empty = 0;
    
    for(ip = &icache.inodes[0]; ip < &icache.inodes[NINODE]; ip++) {
        if(ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref++;
            return ip;
        }
        if(empty == 0 && ip->ref == 0)
            empty = ip;
    }
    
    if(empty == 0) {
        printf("No Empty Inodes\n");
        exit(1);
    }
    
    ip = empty;
    ip->dev = dev;
    ip->inum = inum;
    ip->ref = 1;
    ip->flags = 0;
    return ip;
}







