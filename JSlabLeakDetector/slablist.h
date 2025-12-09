#include "DataStructures.h"
#include <stdio.h>
#include <stdlib.h>
//#include_next "DataStructures.h"

void readslabs(struct snapshot *s){
    FILE *Stream = fopen("/proc/slabinfo", "r");
    if(!Stream){
        perror("Failed to open /proc/slabinfo");
        return;
    }
    char Buffer[256];
    if (Stream) {//why fp is show as int * fp in recommendation in zed
        //sscanf(), fgets, fscanf how many are there? how to optimally choose in betweem
        while (fgets(Buffer,sizeof(Buffer),Stream)!=NULL) {
            char field[64];
            sscanf(Buffer, "%s", field);
        }
        fclose(Stream);
    }
}

void slablisttraverse(slabinfo *s){
    //TODO: Implement list traversal
}

struct snapshot* init_slab_list(){
    // Allocate snapshot structure
    struct snapshot *slabs = (struct snapshot*)malloc(sizeof(struct snapshot));
    if (!slabs) {
        perror("Failed to allocate snapshot");
        return NULL;
    }
    
    // Allocate list node
    slabs->l = (list*)malloc(sizeof(list));
    if (!slabs->l) {
        perror("Failed to allocate list node");
        free(slabs);
        return NULL;
    }
    
    slabs->enumtype = SLABINFO;
    
    // Initialize circular doubly-linked list
    slabs->l->prev = slabs->l;
    slabs->l->next = slabs->l;

    return slabs;
}

void init_slab_list_noptr(){
    // Allocate snapshot structure
    struct snapshot *slabs = (struct snapshot*)malloc(sizeof(struct snapshot));
    if (!slabs) {
        perror("Failed to allocate snapshot");
        return;
    }
    
    // Allocate list node
    slabs->l = (list*)malloc(sizeof(list));
    if (!slabs->l) {
        perror("Failed to allocate list node");
        free(slabs);
        return;
    }
    
    slabs->enumtype = SLABINFO;
    
    // Initialize list pointers
    // If headvmstat is NULL, this is the first node
    if (headvmstat == NULL) {
        headvmstat = slabs->l;
        headvmstat->next = headvmstat;
        headvmstat->prev = headvmstat;
    } else {
        // Insert at the beginning
        slabs->l->next = headvmstat;
        slabs->l->prev = headvmstat->prev;
        headvmstat->prev->next = slabs->l;
        headvmstat->prev = slabs->l;
    }
}
