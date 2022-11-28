#pragma once
#include "main.h"

struct node {
    char name[DIRSIZ];
    uint dev;
    void *ptr;
};

struct htable {
    uint capacity;
    uint nkeys;
    struct node nodes[NBUF];
};

uint htableremove(struct htable *t, uint key, uint dev);
uint htableinsert(struct htable *t, uint sector, uint dev);
struct node* htablesearch(struct htable *t, uint sector, uint dev);

