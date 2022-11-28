#pragma once
#include "buf.h"

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

void loginit(void);
void initlog(uint dev);

void begin(void);
void commit(void);
void log_write(struct buf *b);
