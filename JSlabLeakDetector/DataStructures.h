//
// Created by anamitra on 13/11/25.
//

#ifndef KMEMLEAK_DATASTRUCTURES_H
#define KMEMLEAK_DATASTRUCTURES_H

typedef struct {
  //datas made available by 'cat /proc/slabinfo'
  struct list *list;
}slabinfo;

typedef struct {
  //datas made available by 'cat /proc/vmstat'
  struct list *list;
}vmstat;

typedef struct {
  //datas made available by 'cat /proc/buddyinfo'
  struct list *list;
}buddyinfo;

typedef struct {
    struct list *prev, *next;
}list;

#endif //KMEMLEAK_DATASTRUCTURES_H