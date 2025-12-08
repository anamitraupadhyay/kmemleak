#include "DataStructures.h"
#include <stdio.h>
#include <stdlib.h>
//#include_next "DataStructures.h"

void readslabs(snapshot *s){
    FILE *Stream = fopen_s("/proc/slabinfo","r");
    if(!Stream){
        return;
    }
    int someslabdata;
    char Buffer[256];
    if (Stream) {//why fp is show as int * fp in recommendation in zed
        //sscanf(), fgets, fscanf how many are there? how to optimally choose in betweem
        while (fgets(Buffer,sizeof(Buffer),*Stream)!=NULL) {
            sscanf_s("%s",Buffer);
        }
    }
}

void slablisttraverse(slabinfo *s){
    //
}

snapshot* init_slab_list(){//void init_slab_list is it required to have snapshot* instead of void
    snapshot *slabs = (snapshot*)malloc(sizeof(slabinfo));
    
    slabs->enumtype = SLABINFO;
    
    /*(&headvmstat)->prev = (&slabs);
    (&headvmstat)->next = (&slabs);*/
    
    //Now, *connect* the new slab list to the other lists.
    slabs->l->prev = &headslabinfo;         // Connect the new slab list to the headslabinfo list.
    slabs->l->next = &headslabinfo;     // Connect to headslabinfo's next list

    return slabs;
}

void init_slab_list_noptr(){//void init_slab_list is it required to have snapshot* instead of void
    snapshot *slabs = (snapshot*)malloc(sizeof(slabinfo));
    
    slabs->enumtype = SLABINFO;
    
    /*(&headvmstat)->prev = (&slabs);
    (&headvmstat)->next = (&slabs);*/
    
    //Now, *connect* the new slab list to the other lists.
    //slabs->l->prev = &headslabinfo;// Connect the new slab list to the headslabinfo list.
    // slabs->l->next = &headslabinfo;// Connect to headslabinfo's next list
    headvmstat->next = slabs;
    headvmstat->prev = NULL;
    

    return;
}
