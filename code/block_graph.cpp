void BlockGraph_InsertEdge(BLOCK_GRAPH *graph, MEMORY_ARENA *arena, int from, int to)
{
    BLOCK_GRAPH_EDGE edge = {};
    edge.from = from;
    edge.to = to;
    PushStruct(arena, BLOCK_GRAPH_EDGE);
    graph->edges[graph->edge_count++] = edge;
}
