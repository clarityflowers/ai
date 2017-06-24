#if !defined(BOARD_H)

enum TileKind { Air, Metal, Wood };

struct TILE
{
    TileKind kind;
    bool32 on_fire;
    int health;
    bool32 connected[4];
};

struct BOARD
{
    TILE* tiles;
    int width, height;
};

#define ALPHA_CUBE_H
#endif
