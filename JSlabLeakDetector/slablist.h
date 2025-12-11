#include "DataStructures.h"
#include <stdio.h>
#include <stdlib.h>
//#include_next "DataStructures.h"

void readslabs(struct snapshot *s) { // when struct was not written-Must use 'struct'
                              // tag to refer to type 'snapshot
  FILE *Stream = fopen("/proc/slabinfo", "r");
  if (!Stream) {
    return;
  }
  int someslabdata;
  char Sparsedword[100];
  char Buffer[256];
  if (Stream) { // why fp is show as int * fp in recommendation in zed
    // sscanf(), fgets, fscanf how many are there? how to optimally choose in
    // betweem
    while (fgets(Buffer, sizeof(Buffer), Stream) !=
           NULL) { // remove the '*' ptr and issue gone, ow yes
                //Stream is already a pointer that is *Stream points to the file
                if (sscanf(Buffer, "%s", Sparsedword) == 1) {
                  printf("%s",Sparsedword);
      }
    }
  }
  fclose(Stream);
  return;
}

void slablisttraverse(slabinfo *s){
    //
}

struct snapshot* init_slab_list(){
    struct snapshot *slabs = (struct snapshot*)malloc(sizeof(struct snapshot));
    
    slabs->enumtype = SLABINFO;
    
    return slabs;
}

void init_slab_list_noptr(){
    struct snapshot *slabs = (struct snapshot*)malloc(sizeof(struct snapshot));
    
    slabs->enumtype = SLABINFO;
    
    return;
}
