//
// Created by anamitra on 13/11/25.
//
//#include <stdio.h>
#include "slablist.h"
//#include <cstddef>

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
  headslabinfo = NULL;
  headslabinfo = NULL;
  headbuddyinfo = NULL;
  // setting all these extern pointers as null to better check status
  init_slab_list_noptr();
  while (argv) {
    // init_slab_list_noptr(); //init should happen before the loop
  }
  return 0;
}
