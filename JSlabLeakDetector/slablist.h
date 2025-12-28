#include "DataStructures.h"
#include <stdio.h>
#include <stdlib.h>
//#include_next "DataStructures.h"

// usage will be
// readslabs(GET_SNAPSHOT(&slabs->l));
// or for simplicity explore ** and call the macro in parameter of the function
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


// usage will be slablisttraverse with starting cache of ptr
// no return type but need some logic to control it 
// and call will be similar to readslabs using GET_SNAPSHOT
// logic: depends as of i need 2 
void slablisttraverse(snapshot *s){
    //
    //GET_SNAPSHOT(s);
}

struct list* init_slab_list(void) {
  struct snapshot *slabs;
  slabs = (struct snapshot *)malloc(sizeof(struct snapshot));

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

  slabs->filedata.svar = (slabinfo*)malloc(sizeof(slabinfo));
  if(!slabs->filedata.svar) return NULL;

  slabs->filedata.svar->list.next = NULL;
  slabs->filedata.svar->list.prev = NULL;

  return &(slabs->l);
}

void init_slab_list_noptr() {

  struct snapshot *slabs;
  slabs = (struct snapshot *)malloc(sizeof(struct snapshot));
  if (!slabs) {
    //free(slabs); //freeing null !makes sense
    return;
  }

  //the macro is only required for traversals not here
  slabs->enumtype = SLABINFO;

  slabs->filedata.svar = malloc(sizeof(*slabs->filedata.svar));
  if (!slabs->filedata.svar) {
    free(slabs);
    return;
  }

  //overrriding the union's behaviour
  //slabs->filedata.bvar = NULL;
  //slabs->filedata.vvar = NULL;

  //initing the fields to set it later
  slabs->l.next = NULL;
  slabs->l.prev = NULL;

  //setting the pointers of *headslabs and *slabs
  if (headslabinfo == NULL) {
    headslabinfo/*->next*/ = &slabs->l; // recovery should happen with GET_SNAPSHOT
    //headslabinfo->prev =&slabs->l; // GET_SNAPSHOT(&slabs->l) is dataype conflict

    slabs->l.prev = headslabinfo;
    slabs->l.next = NULL; // NULL is a macro = ((void*)0)
  } else {
    // above was the case when its empty
    // and due to no malloc of list *l
    // we didnt had to deal with its list pre v and next
    // but here its assumed that it has those entries
    // but better to have a check to proceed
    // as it could be headslabinfo was just pointing to other and didnt had
    // those declarations too
    if (!headslabinfo->next && !headslabinfo->prev) {
      headslabinfo = NULL;//setting it null reffering the 3rd last comments above
      headslabinfo = &slabs->l;
      slabs->l.prev = headslabinfo;
      slabs->l.next = NULL;
    } else {
      // dont set it to null as it has those atttributes and
      // and doing so will result the previous lists to be lost
      headslabinfo->prev = &slabs->l;
      //free(headslabinfo->next);
      headslabinfo->next = &slabs->l;
      slabs->l.prev = headslabinfo;
      headslabinfo = &slabs->l;
    }
  }

  return;
}
