#ifndef VMSTATLIST_H
#define VMSTATLIST_H


#include <stdio.h>

#define INIT_SNAPSHOT_vm 1
#define CHECK_SNAPSHOT_vm 2

#define READ_END 0
#define WRITE_END 1
#define INTERVAL 5

#define INIT_LIST_HEAD(ptr) do { \
(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

//declaring this head globally
extern struct list_head vmstat_head;

typedef struct list_head{
    struct list_head *prev, *next;
}list_head;
typedef struct vmstat{
    struct list_head list_head;
    char name[100];
    unsigned int stats;
}vmstat;
/*struct zone{
    struct vmstat vmstat[100];
};*/
typedef struct diffvm{
    char name[100];
    unsigned int statsdiff;
}diffvm;

struct diffvm list_update_or_add_vmstat(const char *name, unsigned int new_stats);
struct vmstat* list_find_vmstat(const char *name);
void list_add_vmstat(struct vmstat *new_stat);
void init_vmstat_list();
void parse_vmstat();
unsigned int get_vmstat(const char *name);
void show_vmstat_summary();

#include <stdlib.h>
#include <string.h>

struct list_head vmstat_head;

void list_add_vmstat(struct vmstat *new_stat) {
    new_stat->list_head.next = vmstat_head.next;
    new_stat->list_head.prev = &vmstat_head;
    vmstat_head.next->prev = &new_stat->list_head;
    vmstat_head.next = &new_stat->list_head;
}

struct vmstat* list_find_vmstat(const char *name) {
    struct list_head *pos;
    for (pos = vmstat_head.next; pos != &vmstat_head; pos = pos->next) {
        struct vmstat *entry = list_entry(pos, struct vmstat, list_head);
        if (strcmp(entry->name, name) == 0) return entry;
    }
    return NULL;
}

struct diffvm list_update_or_add_vmstat(const char *name, unsigned int new_stats) {
    struct diffvm d;
    strcpy(d.name, name);
    d.statsdiff = 0;

    struct vmstat *entry = list_find_vmstat(name);
    if (entry) {
        d.statsdiff = new_stats - entry->stats;
        entry->stats = new_stats;
    } else {
        struct vmstat *new_entry = malloc(sizeof(struct vmstat));
        strcpy(new_entry->name, name);
        new_entry->stats = new_stats;
        list_add_vmstat(new_entry);
    }

    return d;
}


void init_vmstat_list()
{
    INIT_LIST_HEAD(&vmstat_head);
}

void parse_vmstat()
{
    FILE *fp = fopen("/proc/vmstat", "r");
    if (!fp)
    {
        perror("fopen /proc/vmstat");
        return;
    }
    char name[128];
    unsigned long long val;
    while (fscanf(fp, "%127s %llu", name, &val) == 2)
    {
        list_update_or_add_vmstat(name, (unsigned int)val);
    }
    fclose(fp);
}

unsigned int get_vmstat(const char *name)
{
    struct vmstat *entry = list_find_vmstat(name);
    return entry ? entry->stats : 0;
}

void show_vmstat_summary()
{
    unsigned int memfree = get_vmstat("nr_free_pages");
    unsigned int reclaim = get_vmstat("nr_slab_reclaimable");
    unsigned int unreclaim = get_vmstat("nr_slab_unreclaimable");

    printf("[VMSTAT] free_pages=%u reclaimable=%u unreclaimable=%u\n",
           memfree, reclaim, unreclaim);
}



#endif // VMSTATLIST_H
