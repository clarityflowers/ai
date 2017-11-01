#if !defined(BOARD_H)

enum TileKind { Air, Metal, Wood, Fire, Seed, Sprout, Vine };

struct TILE_OFFSET
{
    int8 x, y;
};

TILE_COORD operator+(TILE_COORD a, TILE_OFFSET b)
{
    TILE_COORD result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

TILE_COORD operator+(TILE_OFFSET a, TILE_COORD b)
{
    TILE_COORD result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

struct BLOCK_DEF
{
    TileKind tile_kind;
    TILE_OFFSET offsets[4];
    int num_tiles;
};

struct TILE
{
    TileKind kind;
    uint32 block;
    uint8 health;
    bool8 on_fire;
    
    uint8 growth;
};

struct GAME_BLOCK
{
    TILE_COORD pos;
    int rotate, kind;
    BLOCK_DEF *def;
    TILE* tiles;
};

struct BOARD
{
    TILE* tiles;
    int width, height;
    int block_count;
    GAME_BLOCK block;
};

#define BOARD_H
#endif
