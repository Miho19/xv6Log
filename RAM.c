#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

#include "RAM.h"


struct device devices[MAXDEVICE];


void RAMinit(void) {
    struct device *d;
    
    memset(&devices, 0, (sizeof devices / sizeof devices[0]) * MAXDEVICE);
    
    d = &devices[0];
    d->deviceID = 0;
    d->size = 1024;
    
   
    
    d->DB = malloc(d->size * sizeof(*d->DB));
    
    if(!d->DB) {
        printf("Failed to init device 1 memory\n");
        exit(1);
    }
    
    memset(d->DB, 0, sizeof (*d->DB) * d->size);
    

    
    d = &devices[1];
    d->deviceID = 1;
    d->size = 512;
    

    
    d->DB = malloc(d->size * sizeof(*d->DB));
    
    
    if(!d->DB) {
        printf("Failed to init device 2 memory\n");
        exit(1);
    }
    
    memset(d->DB, 0, sizeof (*d->DB) * d->size);
    
    
    
}



void wRAM(uint dev, uint sector, const unsigned char *data) {
    struct RAMBUFFER *buffer;
    
    buffer = &devices[dev].DB[sector];
    
    memmove(buffer->sector, data, RAMSIZE);
    
}

void rRAM(uint dev, uint sector, unsigned char *data) {
    
    struct RAMBUFFER *buffer;
    buffer = &devices[dev].DB[sector];
    memmove(data, buffer->sector, RAMSIZE);
    
}
