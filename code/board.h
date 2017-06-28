#if !defined(BOARD_H)

enum TileKind { Air, Metal, Wood, Fire, Seed, Vine };

struct TILE_OFFSET
{
    int8 x, y;
};

struct BLOCK_DEF
{
    TileKind tile_kind;
    TILE_OFFSET offsets[4];
    int num_tiles;
};

struct TILE
{
    TileKind kind;
    bool32 on_fire;
    int health;
    uint32 block;
    bool32 connected[4];
};

struct GAME_BLOCK
{
    int x, y, rotate, kind;
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

#define ALPHA_CUBE_H
#endif
