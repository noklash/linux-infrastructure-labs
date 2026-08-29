#ifndef DISPLAY_H
#define DISPLAY_H

#include "proc.h"

void display_table(struct proc_table *t, int limit);
void display_one(const struct proc_info *p);
void display_tree(struct proc_table *t);
void display_fds(int pid);
void display_maps(int pid);

#endif