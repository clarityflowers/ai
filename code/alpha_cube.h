#if !defined(ALPHA_CUBE_H)

#include "platform.h"
#include "sound.h"
#include "board.h"
#include "block_graph.h"

#define BOARD_WIDTH 32
#define BOARD_HEIGHT 20
#define GAME_HEIGHT 240
#define GAME_WIDTH 256

struct RGBA_COLOR
{
    uint8 r,g,b,a;
};


struct MEMORY_ARENA
{
    memory_index size;
    uint8 *base;
    memory_index used;
};

struct TILE_COORD
{
    int x, y;
};


internal void InitializeArena(MEMORY_ARENA *arena, memory_index size, uint8 *base)
{
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

#define PushStruct(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type *)PushSize_(arena, (count)*sizeof(type))
void * PushSize_(MEMORY_ARENA *arena, memory_index size)
{
    Assert((arena->used + size) <= arena->size);
    void *result = arena->base + arena->used;
    arena->used += size;

    return result;
}

#define PopStruct(arena, type) (type *)PopSize_(arena, sizeof(type))
#define PopArray(arena, count, type) (type *)PopSize_(arena, (count)*sizeof(type))
void * PopSize_(MEMORY_ARENA *arena, memory_index size)
{
    Assert((arena->used - size) >= 0);
    void *result = arena->base + arena->used - size;
    arena->used -= size;

    return result;
}


struct GAME_STATE
{
    MEMORY_ARENA def_arena;
    BLOCK_DEF *block_defs;
    MEMORY_ARENA board_arena;
    RGBA_COLOR palettes[3][4];
    PIXEL_BACKBUFFER buffer;
    uint8 pixels[(2 * GAME_WIDTH * GAME_HEIGHT) / 8];
    BOARD board;
    int frames;
    int safety;
    INSTRUMENT_STATE instrument_state;
    INSTRUMENT instrument;
    AUDIO_CLOCK clock;
    int timer;
    int gravity_timer;
    int beats;
    float32 audio_buffer[1024];
    int random_number_index;
    bool32 gravity;
    bool32 get_next_block;
};

#define ALPHA_CUBE_H
#endif
