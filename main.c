// Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NBUF 10
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
    uint sector;
    uint dev;
    int flags;
};

struct {
    struct buf buf[NBUF];
    struct buf head;
} bcache;

void usbrw(struct buf *b);
void brelse(struct buf *b);
void readsb(uint dev, struct superblock *sb);
struct buf * bread(uint dev, uint sector);
void brelse(struct buf *b);
void bwrite(struct buf *b);


void loginit(void) {
    memset(&logmeta, 0, sizeof logmeta/sizeof logmeta[0]);
    
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

void readhead(uint dev) {
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

void writehead(uint dev) {
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

void recover(uint dev) {
    readhead(dev);
    install_trans(dev);
    logmeta[dev].lh.n = 0;
    writehead(dev);
}


void install_trans(uint dev) {
    struct logmeta *l;
    struct logheader *lh;
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

void begin() {
    
}

void commit() {
    int i;
    struct logmeta *l;
    
    //printf("device 1: %d device 2: %d\n", logmeta[0].lh.n, logmeta[1].lh.n);
   
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
        printf("%d | dev: %d | sector %d |\n", i, b->dev, b->sector);
    }
}


struct buf* bget(uint dev, uint sector) {
    struct buf *b;
    int i;
    //printf("%d | b->dev %d b->sector %d\n", i, b->dev, b->sector);
    //printf("bget: dev: %d sector %d\n", dev, sector);
    loop:
   
    for(b = bcache.head.next, i = 1; b != &bcache.head; b = b->next, i++) {
        
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
    
}

void readsb(uint dev, struct superblock *sb){
    struct buf *b;
    b = bread(dev, 1);
    memmove(sb, b->buffer, sizeof(*sb));
    
    brelse(b);
}




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
    
    initlog(0);
    
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
    initlog(1);
    
}





struct node {
    char name[DIRSIZ];    
};

struct htable {
    uint capacity;
    uint nkeys;
    struct node nodes[NBUF];
};


static uint htable_step(struct htable *t, uint key) {
    return 1 + (key % (t->capacity -1));
}

static uint htable_word_to_int(const char *word) {
    uint result;
    
    result = 0;
    
    while(*word != '\0'){
        result = (*word + 31 * result);
        word++;
    }
    return result;
}


void htabledisplay(struct htable *t) {
    int i;
    
    struct node *n;
   
    for(i = 0; i < t->capacity; i++) {
        
        n = &t->nodes[i];
        
        if(strlen(n->name) == 0)
            continue;
        printf("| %d |\t%s\n", i, n->name);
    }
}

struct node* htablesearch(struct htable *t, const char *key) {
    uint hash;
    uint index;
    uint collisions;
    uint step;
    
    struct node *n;
    
    hash = htable_word_to_int(key);
    index = (hash % t->capacity);
    
    
    n = &t->nodes[index];
    step = htable_step(t, index);
    collisions = 0;
    
    while(strncmp(n->name, key, DIRSIZ) != 0) {
       
        collisions++;
        index = (index + step) % t->capacity;
        if(collisions == t->capacity) break;
        
        n = &t->nodes[index];
        
    }
   
     
    if(collisions == t->capacity) n = 0;
    
      //printf("index %d/%d collisions: %d n->name %s key %s\n", index, t->capacity, collisions,n->name, key);
      
    return n;
}

uint htableinsert(struct htable *t, const char *key) {
    uint hash;
    uint index;
    
    uint collisions;
    uint step;
    
    struct node *n;
    
    hash = htable_word_to_int(key);
    index = (hash % t->capacity);
    
    n = &t->nodes[index];
    collisions = 0;
    step = htable_step(t, index);
    
    while(strlen(n->name) != 0) {
        
        if(strncmp(n->name, key, DIRSIZ) == 0)
            return 1;
        
        index = (index + step) % t->capacity;
        n = &t->nodes[index];
        collisions++;
        if(collisions == t->capacity) break;
    }
    
    
    if(collisions == t->capacity) {
        return 0;
    }
    
   
    strncpy(n->name, key, DIRSIZ);
    t->nkeys++;
    return 1;
    
}


uint htableremove(struct htable *t, const char *key) {
    struct node *n;
    
    n = htablesearch(t, key);
    
    if(!n) return 0;
    
    strncpy(n->name, "", DIRSIZ);
    
    return 1;
}


void initbcache() {
    memset(&bcachetable, 0, sizeof(struct htable));
    bcachetable.capacity = NBUF;
    
}

struct htable bcachetable;



void initFS(void) {
    binit();
    loginit();
    RAMinit();
    
}

void shutdownFS(void) {
    free(devices[0].DB);
    free(devices[1].DB);
}

void shutdown(void){
    shutdownFS();
}

int main() {
   
    return 0;
}
