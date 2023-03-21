//
// Created by James on 2023/3/9.
//
#include "graph.h"

void insertAdj(struct VNode* v1, struct VNode* v2) {
    if (v1->firstAdj == NULL) {
        v1->firstAdj = (struct AdjNode*)malloc(sizeof(struct AdjNode));
        if (v1->firstAdj == NULL) {printf("overflow");exit(1);}
        v1->firstAdj->next = NULL;
        v1->firstAdj->vnode = v2;
    } else {
        struct AdjNode* curAdj = v1->firstAdj;
        while (curAdj->next) {
            curAdj = curAdj->next;
        }
        curAdj->next = (struct AdjNode*)malloc(sizeof(struct AdjNode));
        if (curAdj->next == NULL) {printf("overflow");exit(1);}
        curAdj->next->next = NULL;
        curAdj->next->vnode = v2;
    }
}

struct VNode** buildGraph(struct op_node** copyarr, int len) {
    struct VNode** graph = (struct VNode**)malloc(sizeof(struct VNode*) * len);
    if (!graph) return NULL;
    for (int i = 0; i < len; ++i) {
        graph[i] = (struct VNode*)malloc(sizeof(struct VNode));
        if (!graph[i]) return NULL;
        graph[i]->val = i;
        graph[i]->firstAdj = NULL;
        graph[i]->removed = 0;
    }

    for (uint64_t i = 0; i < len; ++i) {
        for (uint64_t j = i + 1; j < len; ++j) {
            int64_t fi = copyarr[i]->op->from;
            int64_t ti = copyarr[i]->op->to;
            int64_t li = copyarr[i]->op->len;
            int64_t fj = copyarr[j]->op->from;
            int64_t tj = copyarr[j]->op->to;
            int64_t lj = copyarr[j]->op->len;

            if (!((fi+li-1 < tj) || (fi > tj+lj-1))) {
                insertAdj(graph[i], graph[j]);
            }
            if (!((fj+lj-1 < ti) || (fj > ti+li-1))) {
                insertAdj(graph[j], graph[i]);
            }
        }
    }

    printf("graph constructed\n");
    return graph;
}

void dfsVisit(struct VNode** graph, int node, int* visit, int* father, int len, struct op_node* add_head, struct op_node** copyarr) {
    visit[node] = 1;

    for (int i = 0; i < len; ++i) {
        if (i != node) {
            int found = 0;
            struct AdjNode* cur = graph[i]->firstAdj;
            while (cur) {
                if (cur->vnode->val == i) {
                    found = 1;
                    break;
                }
                cur = cur->next;
            }

            if (found) {
                if (visit[i] == 1 && i != father[node]) {
                    // local minimum
                    int tmp = node;
                    int min = -1;
                    int min_node = -1;
                    while (tmp != i) {
                        if (min == -1 || copyarr[tmp]->op->len < min) {min = copyarr[tmp]->op->len; min_node = tmp;}
                        tmp = father[tmp];
                    }
                    if (min == -1 || copyarr[tmp]->op->len < min) {min = copyarr[tmp]->op->len; min_node = tmp;}

                    // todo: convert copy to add
                    copyarr[min_node]->op->optype = ADD;
                    copyarr[min_node]->op->extra = (uint8_t*)malloc(sizeof(uint8_t) * copyarr[min_node]->op->len);
                    if (copyarr[min_node]->op->extra == NULL) {printf("overflow");exit(1);}
                    const int64_t start = copyarr[min_node]->op->from;
                    for (int64_t off = 0; off < copyarr[min_node]->op->len; ++off) {
                        copyarr[min_node]->op->extra[off] = copyarr[min_node]->op->old[start+off] + copyarr[min_node]->op->diff[off];
                    }
                    op_node_insert(add_head, copyarr[min_node]);

                    graph[min_node]->removed = 1;

                } else if (visit[i] == 0) {
                    father[i] = node;
                    dfsVisit(graph, i, visit, father, len, add_head, copyarr);
                }
            }
        }
    }
    visit[node] = 2;
}

// return an add-linkedlist converted from removed copy nodes
// https://www.cnblogs.com/TenosDoIt/p/3644225.html algorithm 2
struct op_node* removeRings(struct VNode** graph, int len, struct op_node** copyarr) {
    int father[len], visit[len];
    for (int i = 0; i < len; ++i) {
        father[i] = -1;
        visit[i] = 0;
    }
    struct op_node* add_head = (struct op_node*)malloc(sizeof(struct op_node));
    if (!add_head) {printf("overflow");exit(1);}

    for (int i = 0; i < len; ++i) {
        if (visit[i] == 0) {
            dfsVisit(graph, i, visit, father, len, add_head, copyarr);
        }
    }

    return add_head;
}

// https://www.cnblogs.com/skywang12345/p/3711489.html
int* toposort(struct VNode** graph, int len) {
    int* res = (int*)malloc(sizeof(res)*len);
    if (res == NULL) {printf("overflow");exit(1);}
    int pos = 0;

    int i,j;
    int index = 0;
    int head = 0;           // 辅助队列的头
    int rear = 0;           // 辅助队列的尾
    int *queue;             // 辅组队列
    int *ins;               // 入度数组
    int *tops;             // 拓扑排序结果数组，记录每个节点的排序后的序号。
    int num = len;
    struct AdjNode* node;

    ins  = (int *)malloc(num*sizeof(int));  // 入度数组
    tops = (int *)malloc(num*sizeof(int));// 拓扑排序结果数组
    queue = (int *)malloc(num*sizeof(int)); // 辅助队列
    if (ins==NULL || tops==NULL || queue==NULL) {printf("overflow");exit(1);}
    memset(ins, 0, num*sizeof(int));
    memset(tops, 0, num*sizeof(int));
    memset(queue, 0, num*sizeof(int));

    // 统计每个顶点的入度数
    for(i = 0; i < num; i++)
    {
        if (graph[i]->removed) {
            ins[i] = -1;
            continue;
        }

        node = graph[i]->firstAdj;
        while (node != NULL) {
            if (!node->vnode->removed) {
                ins[node->vnode->val]++;
            } else {
                ins[node->vnode->val] = -1;
            }
            node = node->next;
        }
    }

    // 将所有入度为0的顶点入队列
    for(i = 0; i < num; i++)
        if(ins[i] == 0)
            queue[rear++] = i;          // 入队列

    while (head != rear) {
        j = queue[head++];
        tops[index++] = graph[j]->val;
        node = graph[j]->firstAdj;

        while (node != NULL) {
            if (node->vnode->removed) continue;

            ins[node->vnode->val]--;
            if (ins[node->vnode->val] == 0) {
                queue[rear++] = node->vnode->val;
            }

            node = node->next;
        }
    }

    tops[index++] = -1;

    //free(queue);
    free(ins);
    return tops;
}