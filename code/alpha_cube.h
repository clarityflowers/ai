#if !defined(ALPHA_CUBE_H)

#include <stdint.h>
#include <SDL.h>

#if ALPHA_CUBE_SLOW
#define Assert(Expression) \
    if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Pi32 3.1415926539f
#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct GAME_BUTTON_STATE
{
    bool32 is_down;
    int transitions;
};

struct GAME_CONTROLLER_INPUT
{
    bool32 is_connected;

    union
    {
        GAME_BUTTON_STATE buttons[3];
        struct
        {
            GAME_BUTTON_STATE move_right;
            GAME_BUTTON_STATE move_left;
            GAME_BUTTON_STATE drop;
        };
    };
};

struct GAME_INPUT
{
    GAME_CONTROLLER_INPUT controllers[1];
};

struct PIXEL_BACKBUFFER
{
    void *pixels;
    int h, w, pitch, depth;
};

struct GAME_MEMORY
{
    bool32 is_initialized;

    uint64 permanent_storage_size;
    void* permanent_storage; // NOTE(casey): REQUIRED to be cleared to zero at startup
    int permanent_storage_offset;

    uint64 transient_storage_size;
    void* transient_storage; // NOTE(casey): REQUIRED to be cleared to zero at startup
    int transient_storage_offset;
};

#define GAME_UPDATE_AND_RENDER(name) void name( GAME_MEMORY *memory, PIXEL_BACKBUFFER *render_buffer, GAME_INPUT *input, uint32 delta_time )
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

struct RGBA_COLOR
{
    uint8 r,g,b,a;
};

struct GAME_STATE
{
    RGBA_COLOR palettes[1][4];
    PIXEL_BACKBUFFER buffer;
    bool32 board[20][10];
    int frames;
    int block_x;
    int block_y;
    float fall_speed;
    float fall_time;
    bool32 fall_quickly;
};

#define ALPHA_CUBE_H
#endif
