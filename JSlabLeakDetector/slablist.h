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

void slablisttraverse(snapshot *s){
    //
}

struct snapshot* init_slab_list(){
    struct snapshot *slabs = (struct snapshot*)malloc(sizeof(struct snapshot));
    
    slabs->enumtype = SLABINFO;
    
    return slabs;
}

void init_slab_list_noptr() {

  struct snapshot *slabs = (struct snapshot *)malloc(sizeof(struct snapshot));
  if (!slabs) {
    free(slabs);
    return;
  }
  slabs->l = *(list *)malloc(sizeof(list)); // Assigning to 'list'
                                           // from incompatible type 'list *'
  // quick fix was to add * at "" = * "";
  if (!slabs->l) {
    free(&slabs->l); // Attempt to call free on non-heap object 'l'
    return;
  }

  if (headslabinfo == NULL) {
    headslabinfo->next = &slabs->l; // recovery should happen with GET_SNAPSHOT
    headslabinfo->prev =
        &slabs->l; // GET_SNAPSHOT(&slabs->l) is dataype conflict

    slabs->l.prev = headslabinfo;
    slabs->l.next = NULL; // NULL is a macro = ((void*)0)
  } else {
  //
  }

  //

  slabs->enumtype = SLABINFO; // invalid and doesnt happen as for proper linking
                              // the macro is required

  return;
}
