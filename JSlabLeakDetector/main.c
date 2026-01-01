//
// Created by anamitra on 13/11/25.
//
//#include <stdio.h>
#include "slablist.h"

struct list *headslabinfo = NULL;
struct list *headbuddyinfo = NULL;
struct list *headvmstat = NULL;

/*static inline void dlist_head(struct list *volatile headslabinfo,
    struct list *volatile headvmstat,
    struct list *volatile headbuddyinfo) {
        headbuddyinfo = NULL;
        headslabinfo = NULL;
        headvmstat = NULL;
    }*/

int main(
    const int argc,
    char *argv
        [] /*for terminal inputsthen whats for int argc*/) { // when void
                                                             // main(void) is
                                                             // done the zed
                                                             // clang is showing
                                                             // return type of
                                                             // main is not int
                                                             // char* argv[],
                                                             // int argc shown
                                                             // the 1st should
                                                             // always be int
  /*headslabinfo = NULL; make it unavailabale for the due scope for the functions to use
  headslabinfo = NULL; need to do this in outside of any function scope i.e global
  headbuddyinfo = NULL;*/
  // setting all these extern pointers as null to better check status => turn
  // out wrong
  // for clean management separating the global fallback and local flow
  // local inits in terms of both style that is
  // struct list* headlslabinfo
  struct list *headslablocal, 
  *headbuddylocal, 
  *headvmlocal = NULL;
  
  headbuddylocal = init_slab_list();
  headbuddylocal = init_buddy();
  headvmlocal = init_vm();
  
  if (!headslablocal 
      && !headbuddylocal
      && !headvmlocal) {
    // global fallback for init
    init_slab_list_noptr();
    while (argv) {
      // init_X_noptr(); //init should happen before the loop
    }
  } else {
    // normal local init goes right
    // but the main problem is if the main exec block failed to execed
    // then it will be execing though its in main block and it
    // doesnt make sense and what condition to set in if block
    // struct list* headslab = init_slab_list(); => wrong
    // and many other inits not here
    // while loops for normal operation encased here
    while (argv) {
      // init_X(); //init should happen before the loop
    }
  }
  return 0;
}
