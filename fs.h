#pragma once
#include "main.h"

struct superblock {
    short size;
    short nlog;
    short nblocks;
    short ninodes;
};

struct dinode {
    short type;
    short major;
    short minor;
    short nlink;
    uint size;
    uint addrs[NDIRECT + 1];
};

struct inode {
    uint dev;
    uint inum;
    int ref;
    int flags;
    
    short type;
    short major;
    short minor;
    short nlink;
    uint size;
    uint addrs[NDIRECT + 1];
};

struct dirent {
    uint inum;
    char name[DIRSIZ];
};



void readsb(uint dev, struct superblock *sb);
