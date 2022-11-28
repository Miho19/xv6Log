#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

#include "buf.h"
#include "htable.h"
#include "RAM.h"


struct {
    struct buf buf[NBUF];
    struct buf head;
} bcache;

struct htable bcachehtable;

void binit(void) {
    struct buf *b;
    int i;
    
    
    memset(&bcache, 0, sizeof bcache);
    
    bcache.head.next = &bcache.head;
    bcache.head.prev = &bcache.head;
    
    for(b = bcache.buf, i = 0; b < bcache.buf + NBUF; b++, i++) {
        
        b->next = &bcache.head;
        b->prev = bcache.head.prev;
        bcache.head.prev->next = b;
        bcache.head.prev = b;
        
        b->dev = -1;
        b->sector = -1;
        memset(b->buffer, 0, sizeof b->buffer/sizeof b->buffer[0]);
    }
    
    memset(&bcachehtable, 0, sizeof(struct htable));
    bcachehtable.capacity = NBUF;
    
}

void display(void) {
    struct buf *b;
    int i;
    
    for(i=1, b = bcache.head.next; b != &bcache.head; b = b->next, i++) {
        printf("%d | dev: %d | sector %d |\n", i, b->dev, b->sector);
    }
}


struct buf* bget(uint dev, uint sector) {
    struct buf *b;
    struct node *n;
    n = 0;
    loop:
    n = htablesearch(&bcachehtable, sector, dev);
    if(n) {
        b = (struct buf *)n->ptr;
        
        if(!(b->flags & B_BUSY)) {
            b->flags |= B_BUSY;
            return b;
        }
        
        goto loop;
    }

    
    for(b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if(!(b->flags & B_BUSY) && !(b->flags & B_DIRTY)) {
            
            if(!htableremove(&bcachehtable, b->sector, b->dev)) {
                if(!(b->sector == -1 && b->dev == -1)) {
                     printf("bget removing htable\n");
                     exit(1);
                }
            }
        
            if(!htableinsert(&bcachehtable, sector, dev)){
                printf("bget adding to htable\n");
                exit(1);
            }
            
            n = htablesearch(&bcachehtable, sector, dev);
            
            if(!n) {
                printf("bget searching error\n");
                exit(1);
            }
            n->ptr = b;
            b->dev = dev;
            b->sector = sector;
            b->flags = B_BUSY;
            
            return b;
        }             
    }    
    return 0;
    
}




struct buf * bread(uint dev, uint sector) {
    struct buf *b;
    
    b = bget(dev, sector);
    
    
    if(!(b->flags & B_VALID)) {
        usbrw(b);        
    }
    
    return b;
}

void brelse(struct buf *b) {
    
    if(!(b->flags & B_BUSY)) {
        printf("Buffer is not busy!\n");
        return;
    }
    
    b->next->prev = b->prev;
    b->prev->next = b->next;

    
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    
    bcache.head.next->prev = b;
    bcache.head.next = b;
    
    b->flags &= ~B_BUSY;
    
}

void bwrite(struct buf *b) {
    
    if( (b->flags & B_BUSY) == 0){
        printf("bwrite: Buffer not busy!\n");
        return;
    }
    
    b->flags |= B_DIRTY;
    usbrw(b);
    
    
}


void printbuf(int index, struct buf *b) {
    printf("| %d | dev: %d sector %d\n", index, b->dev, b->sector);
}


void usbrw(struct buf *b){
    
    if( (b->flags & B_DIRTY) ) {
        wRAM(b->dev, b->sector, b->buffer);
        b->flags &= ~B_DIRTY;
    } else {
        rRAM(b->dev, b->sector, b->buffer);
        b->flags |= B_VALID;
    }
}
