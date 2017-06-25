#if !defined(BLOCK_GRAPH_H)

struct BLOCK_GRAPH_EDGE;

struct BLOCK_GRAPH_NODE
{
    bool32 discovered;
    int edge_count;
    BLOCK_GRAPH_EDGE *edges;
};

struct BLOCK_GRAPH_EDGE
{
    int from;
    int to;
};

struct BLOCK_GRAPH
{
    BLOCK_GRAPH_NODE *nodes;
    BLOCK_GRAPH_EDGE *edges;
    int edge_count;
};

#define BLOCK_GRAPH_H
#endif
