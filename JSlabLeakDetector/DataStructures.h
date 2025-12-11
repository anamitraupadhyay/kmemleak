//
// Created by anamitra on 13/11/25.
//

#ifndef KMEMLEAK_DATASTRUCTURES_H
#define KMEMLEAK_DATASTRUCTURES_H

#include <stddef.h>

#define getsnapshot(ptr) ((snapshot*)((char*)(ptr)-offsetof(snapshot,l)))

typedef struct list{
    struct list *prev, *next;
}list;

typedef struct {
  //datas made available by 'cat /proc/slabinfo'
  /*struct*/ list list;
}slabinfo;

typedef struct {
  //datas made available by 'cat /proc/vmstat'
  struct list list;
}vmstat;

extern struct list *headvmstat;//would have been lot easier
extern struct list *headslabinfo;//if the ptr instead of
extern struct list *headbuddyinfo;//list it was snapshot's

typedef struct {
  //datas made available by 'cat /proc/buddyinfo'
  struct list list;
}buddyinfo;

/*
┌─────────────────────────────────────────────────────────┐
│         SNAPSHOT (Common Container)                     │
│  ┌────────────────┬────────────────┬────────────────┐   │
│  │   Slab Data    │  Vmstat Data   │  Buddy Data    │   │
│  │ (Independent)  │ (Independent)  │ (Independent)  │   │
│  └────────────────┴────────────────┴────────────────┘   │
└─────────────────────────────────────────────────────────┘
        ↓              ↓              ↓
    Pipeline 1     Pipeline 2     Pipeline 3
    (parallel)     (parallel)     (parallel)
        ↓              ↓              ↓
   Parse Slab    Parse Vmstat   Parse Buddy
   Analyze Slab  Analyze Vmstat  Analyze Buddy
        ↓              ↓              ↓
    Slab Result   Vmstat Result  Buddy Result
        └──────────────┬──────────────┘
                       ↓
              Correlation Layer
                       ↓
           Final Leak Assessment

*/

typedef enum filetype{
    SLABINFO,
    BUDDYINFO,
    VMSTAT
}filetype;

struct snapshot{//offcourse i didnt typedef'd it and refering would require struct keyword
    filetype enumtype;
    list l;//never do *l its pointing out of scope of snapshot malloc
  union filedata{
    /*struct*/ slabinfo *svar; // when did struct slabinfo svar-Field has incomplete
                          // type 'struct slabinfo'
    buddyinfo *bvar; // with no struct keyword it shows declaration
                    // does not declare anything
    vmstat *vvar;    // with struct* it displays declaration of anonymous struct
                 // must be definition
  }filedata;
};


#endif //KMEMLEAK_DATASTRUCTURES_H
