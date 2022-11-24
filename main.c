// Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NBUF 5
#define RAMSIZE 512

#define B_BUSY 1
#define B_DIRTY 4
#define B_VALID 2

#define LOGSIZE 10
#define MAXDEVICE 2

typedef unsigned int uint;

struct superblock {
    short size;
    short nlog;
    short nblocks;
    short ninodes;
};


struct RAMBUFFER {
    unsigned char sector[RAMSIZE];
};

struct device {
    uint deviceID;
    uint size;
    struct RAMBUFFER *DB;
};

struct device devices[MAXDEVICE];

void RAMinit(void) {
    struct device *d;
    struct superblock sb;
    
    memset(&devices, 0, sizeof devices / sizeof devices[0]);
    
    d = &devices[0];
    d->deviceID = 0;
    d->size = 1024;
    
   
    
    d->DB = malloc(d->size * sizeof(*d->DB));
    
    if(!d->DB) {
        printf("Failed to init device 1 memory\n");
        exit(1);
    }
    
    memset(d->DB, 0, sizeof (*d->DB) * d->size);
    memset(&sb, 0, sizeof sb);
    
    sb.size = d->size;
    sb.nblocks = 985;
    sb.ninodes = 200;
    sb.nlog = 10;
    
    memmove(&d->DB[1], &sb, sizeof sb);
    
    d = &devices[1];
    d->deviceID = 1;
    d->size = 512;
    
   
    
    d->DB = malloc(d->size * sizeof(*d->DB));
    
    if(!d->DB) {
        printf("Failed to init device 2 memory\n");
        exit(1);
    }
    
    memset(d->DB, 0, sizeof (*d->DB) * d->size);
    memset(&sb, 0, sizeof sb);
    
    sb.size = d->size;
    sb.nblocks = 485;
    sb.ninodes = 120;
    sb.nlog = 10;
    
    memmove(&d->DB[1], &sb, sizeof sb);
    
    
}



struct logheader {
    int n;
    int sector[LOGSIZE];
};

struct logmeta {
  int start;
  int size;
  int busy;
  int dev;
  struct logheader lh;
};

struct logmeta logmeta[MAXDEVICE];



struct buf {
    struct buf *next;
    struct buf *prev;
    unsigned char buffer[RAMSIZE];
    int sector;
    int dev;
    int flags;
};

struct {
    struct buf buf[NBUF];
    struct buf head;
} bcache;

void usbrw(struct buf *b);
void brelse(struct buf *b);
void readsb(uint dev, struct superblock *sb);

void initlog(void) {
    struct superblock sb;
    memset(&sb, 0, sizeof sb);
    readsb(0, &sb);
    
    memset(&logmeta, 0, sizeof logmeta/sizeof logmeta[0]);
    
    
}




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
}

void display(void) {
    struct buf *b;
    int i;
    
    for(i=1, b = bcache.head.next; b != &bcache.head; b = b->next, i++) {
        printf("%d | dev: %d | sector %d | buffer %d \n", i, b->dev, b->sector, b->buffer);
    }
}

struct buf* bget(uint dev, uint sector) {
    struct buf *b;
    
    loop:
    for(b = bcache.head.next; b != &bcache.head; b = b->next) {
        if(b->dev == dev && b->sector == sector) {
            
            if(!(b->flags & B_BUSY)) {
                b->flags |= B_BUSY;
                return b;
            }
            
            goto loop;               
        }
    }

    for(b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if(!(b->flags & B_BUSY) && !(b->flags & B_DIRTY)) {
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
    
    if(!(b->flags * B_BUSY)) {
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



void wRAM(uint dev, uint sector, unsigned char *data) {
    struct RAMBUFFER *buffer;
    
    buffer = &devices[dev].DB[sector];
    
    memmove(buffer->sector, data, RAMSIZE);
    
}

void rRAM(uint dev, uint sector, unsigned char *data) {
    
    struct RAMBUFFER *buffer;
    
    buffer = &devices[dev].DB[sector];
    
    memmove(data, buffer->sector, RAMSIZE);
    
}

void usbrw(struct buf *b){
    
    if( (b->flags & B_DIRTY) ) {
        wRAM(b->dev, b->sector, &b->buffer);
        b->flags &= ~B_DIRTY;
    } else {
        rRAM(b->dev, b->sector, &b->buffer);
        b->flags |= B_VALID;
    }
}

void bwrite(struct buf *b) {
    
    if( (b->flags & B_BUSY) == 0){
        printf("bwrite: Buffer not busy!\n");
        return;
    }
    
    b->flags |= B_DIRTY;
    usbrw(b);
    
    
}


void test(void){
    const char message[] = "Hello World!";
    
    struct buf *b;
    
    struct superblock sb;
    
    b = bread(0, 46);
    printf("b->sector %d b->dev %d b->buffer %s\n", b->sector, b->dev, b->buffer);
    memmove(b->buffer, message, strlen(message) + 1);
    bwrite(b);
    brelse(b);
    
    b = bread(0, 46);
    printf("b->sector %d b->dev %d b->buffer %s\n", b->sector, b->dev, b->buffer);
    brelse(b);
    
    
    memset(&sb, 0, sizeof sb);
    readsb(1, &sb);
    
    printf("superblock\nsb.size %d\nsb.nblocks %d\nsb.ninodes %d\nsb.nlog %d\n", sb.size, sb.nblocks, sb.ninodes, sb.nlog);
    
    
}

void readsb(uint dev, struct superblock *sb){
    struct buf *b;
    
    b = bread(dev, 1);
    
    memmove(sb, b->buffer, sizeof(*sb));
    
    brelse(b);
}

void shutdown(void){
    free(devices[0].DB);
    free(devices[1].DB);
}


int main() {
    // Write C code here
    
    
    binit();
    RAMinit();
    
    test();
    shutdown();
    return 0;
}
