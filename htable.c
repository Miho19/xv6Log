
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "htable.h"
#include <stdio.h>

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

void htabledisplay(struct htable *t, void(*printfunc)(int, void *)) {
    uint i;
    
    struct node *n;
   
    for(i = 0; i < t->capacity; i++) {
        
        n = &t->nodes[i];
        
        if(strlen(n->name) == 0)
            continue;
        printfunc(i, (void *)n->ptr);
    }
}



struct node* htablesearch(struct htable *t, uint sector, uint dev) {
    uint hash;
    uint index;
    uint collisions;
    uint step;
    
    struct node *n;
    
    char name[DIRSIZ];
    memset(&name, 0, DIRSIZ);
    sprintf(name, "dev%dsector%d", dev, sector);
    
    hash = htable_word_to_int(name);
    index = (hash % t->capacity);
    
   
    
    n = &t->nodes[index];
    step = htable_step(t, index);
    collisions = 0;
    
    while(1) {
       
       
       if((strncmp(n->name, name, DIRSIZ) == 0) && (n->dev == dev)) {
           break;
       }
       
        collisions++;
        index = (index + step) % (t->capacity);
        if(collisions == t->capacity) break;
        
        n = &t->nodes[index];
        
    }
    
     
   
    if(collisions == t->capacity) n = 0;
    
      
    return n;
}



uint htableinsert(struct htable *t, uint sector, uint dev) {
    uint hash;
    uint index;
    
    uint collisions;
    uint step;
    
    struct node *n;
    
    char name[DIRSIZ];
    memset(&name, 0, DIRSIZ);
    sprintf(name, "dev%dsector%d", dev, sector);
    
    hash = htable_word_to_int(name);
    index = (hash % t->capacity);
    
    n = &t->nodes[index];
    collisions = 0;
    step = htable_step(t, index);
    
    while(strlen(n->name) != 0) {
        
        if(strncmp(n->name, name, DIRSIZ) == 0 && n->dev == dev)
            return 1;
        
        index = (index + step) % t->capacity;
        n = &t->nodes[index];
        collisions++;
        if(collisions == t->capacity) break;
    }
    
    if(collisions == t->capacity) {
        return 0;
    }
    
    strncpy(n->name, name, DIRSIZ);
    n->dev = dev;
    t->nkeys++;
    return 1;
    
}



uint htableremove(struct htable *t, uint key, uint dev) {
    struct node *n;
    
    n = htablesearch(t, key, dev);
    
    if(!n) return 0;
    
    strncpy(n->name, "", DIRSIZ);
    n->ptr = 0;

    return 1;
}
