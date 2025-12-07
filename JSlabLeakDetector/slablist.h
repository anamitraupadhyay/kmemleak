#include "DataStructures.h"
#include <stdio.h>
//#include_next "DataStructures.h"

void readslabs(snapshot *s){
    FILE *Stream = fopen("/proc/slabinfo","r");
    if(!Stream){
        return;
    }
    int someslabdata;
    char Buffer[256];
    if (Stream) {//why fp is show as int * fp in recommendation in zed
        //sscanf(), fgets, fscanf how many are there? how to optimally choose in betweem
        while (fgets(Buffer,sizeof(Buffer),*Stream)!=NULL) {
            sscanf_s(Buffer, "%s",s->enumtype);
        }
    }
}

void slablisttraverse(slabinfo *s){
    //
}
