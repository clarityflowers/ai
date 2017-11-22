#if !defined(VECTOR_H)

struct V2
{
    float x, y;
};

struct Coord
{
    int x,y;
};

struct TileCoord
{
    int x,y;
};

struct BlockCoord
{
    int x,y;
};

#pragma region hello

#pragma endregion hello

struct Rect
{
    Coord pos;
    int w, h;
};

struct TileRect
{
    TileCoord pos;
    int w, h;
};

#define VECTOR_H
#endif