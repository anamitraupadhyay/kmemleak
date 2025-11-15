//
// Created by anamitra on 13/11/25.
//

#ifndef KMEMLEAK_DATASTRUCTURES_H
#define KMEMLEAK_DATASTRUCTURES_H

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

typedef struct {
  //datas made available by 'cat /proc/buddyinfo'
  struct list *list;
}buddyinfo;


#endif //KMEMLEAK_DATASTRUCTURES_H
