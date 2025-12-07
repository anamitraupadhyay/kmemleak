//
// Created by anamitra on 13/11/25.
//

#ifndef KMEMLEAK_DATASTRUCTURES_H
#define KMEMLEAK_DATASTRUCTURES_H

#include <stdlib.h>
typedef struct list{
    struct list *prev, *next;
}list;

typedef struct {
  //datas made available by 'cat /proc/slabinfo'
  /*struct*/ list *list;
}slabinfo;

typedef struct {
  //datas made available by 'cat /proc/vmstat'
  struct list *list;
}vmstat;

extern list headvmstat;
extern list headslabinfo;
extern list headbuddyinfo;

typedef struct {
  //datas made available by 'cat /proc/buddyinfo'
  struct list *list;
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

struct snapshot{
    filetype enumtype;
    list* l;
  union filedata{
      struct slabinfo;
      struct buddyinfo;//with no struct keyword it shows declaration does not declare anything
      struct vmstat;//with struct* it displays declaration of anonymous struct must be definition
  };
};


#endif //KMEMLEAK_DATASTRUCTURES_H
