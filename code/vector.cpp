#include "vector.h"
#include <math.h>

V2 operator+(V2 a, V2 b)
{
    V2 result = {a.x + b.x, a.y + b.y};
    return result;
}

V2 operator+=(V2& a, V2 b)
{
    V2 result = a + b;
    a = result;
    return result;
}

V2 operator*(V2 a, float b)
{
    V2 result = {a.x * b, a.y * b};
    return result;
}

V2 operator/(V2 a, float b)
{
    V2 result = {a.x / b, a.y / b};
    return result;
}

float Length(V2 v)
{
    float result = sqrtf(v.x * v.x + v.y * v.y);
    return result;
}

V2 Normalize(V2 v)
{
    V2 result = {};
    float length = Length(v);
    if (length != 0)
    {
        result = v / Length(v);
    }
    return result;
}


Coord operator+(Coord a, Coord b)
{
    Coord result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

Coord operator+=(Coord& a, Coord b)
{
    Coord result = a + b;
    a = result;
    return result;
}

Coord operator-(Coord a, Coord b)
{
    Coord result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

bool operator==(Coord a, Coord b)
{
    bool result = (a.x == b.x && a.y == b.y);
    return result;
}

Coord ToCoord(TileCoord tile_coord)
{
    Coord result = {tile_coord.x * 8, tile_coord.y * 8};
    return result;
}

int RoundToInt(float x)
{
    int result = (int)round(x);
    return result;
}

BlockCoord ToBlockCoord(V2 vector)
{
    BlockCoord result = {RoundToInt(vector.x), RoundToInt(vector.y)};
    return result;
}

TileCoord ToTileCoord(BlockCoord block_coord)
{
    TileCoord result = {block_coord.x * 2, block_coord.y * 2};
    return result;
}

Coord ToCoord(BlockCoord block_coord)
{
    Coord result = ToCoord(ToTileCoord(block_coord));
    return result;
}


TileCoord operator+(TileCoord a, TileCoord b)
{
    TileCoord result = {a.x + b.x, a.y + b.y};
    return result;
}

TileCoord ToTileCoord(Coord coord)
{
    TileCoord result = {(int)floorf(coord.x / 8.0f), (int)floorf(coord.y / 8.0f)};
    return result;
}

Rect ToRect(TileRect tile_rect)
{
    Rect result = {{tile_rect.pos.x * 8, tile_rect.pos.y * 8}, tile_rect.w * 8, tile_rect.h * 8};
    return result;
}

BlockCoord ToBlockCoord(Coord coord)
{
    BlockCoord result = {coord.x / BLOCK_SIZE, coord.y / BLOCK_SIZE};
    return result; 
}

Coord ToCoord(V2 vector)
{
    V2 v = vector * BLOCK_SIZE;
    Coord result = {RoundToInt(v.x), RoundToInt(v.y)};
    return result;
}