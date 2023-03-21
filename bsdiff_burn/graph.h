//
// Created by James on 2023/3/9.
//

#ifndef BSDIFF_BURN_GRAPH_H
#define BSDIFF_BURN_GRAPH_H

#endif //BSDIFF_BURN_GRAPH_H

#include "bsdiff.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

struct VNode;
struct AdjNode{
    struct VNode* vnode;
    struct AdjNode* next;
};

struct VNode{
    int val;
    int removed;
    struct AdjNode* firstAdj;
};

struct VNode** buildGraph(struct op_node** copyarr, int len);
struct op_node* removeRings(struct VNode** graph, int len, struct op_node** copyarr);
int* toposort(struct VNode** graph, int len);