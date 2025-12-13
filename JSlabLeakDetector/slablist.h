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
  struct snapshot *slabs = (struct snapshot *)malloc(sizeof(struct snapshot));

  /*
  snapshot* init_slab_list(void) {
    // Returns pointer OR NULL on error
}

// Usage
snapshot *slab_head = init_slab_list();
if (!slab_head) {
    // Handle error
    exit(EXIT_FAILURE);
}
  */

  slabs->enumtype = SLABINFO;

  return slabs;
}

void init_slab_list_noptr() {

  struct snapshot *slabs = (struct snapshot *)malloc(sizeof(struct snapshot));
  if (!slabs) {
    free(slabs);
    return;
  }

  //

  slabs->enumtype = SLABINFO; // invalid and doesnt happen as for proper linking
                              // the macro is required

  slabs->filedata.svar = malloc(sizeof(*slabs->filedata.svar));
  if (!slabs->filedata.svar) {
    free(slabs);
    return;
  }
  
  //not needed fields, but is this necessary as its parts of union
  slabs->filedata.bvar = NULL;
  slabs->filedata.vvar = NULL;

  //initing the fields to set it later
  slabs->l.next = NULL;
  slabs->l.prev = NULL;

  //setting the pointers of *headslabs and *slabs
  if (headslabinfo == NULL) {
    headslabinfo->next = &slabs->l; // recovery should happen with GET_SNAPSHOT
    headslabinfo->prev =
        &slabs->l; // GET_SNAPSHOT(&slabs->l) is dataype conflict

    slabs->l.prev = headslabinfo;
    slabs->l.next = NULL; // NULL is a macro = ((void*)0)
  } else {
    //
  }

  return;
}
