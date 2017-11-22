#if !defined(PLATFORM_H)

#include <stdint.h>
#include "vector.h"

#if ALPHA_CUBE_SLOW
#define Assert(Expression) \
    if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Pi32  3.1415926539f
#define Tau32 6.2831853071f
#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;
typedef int8 bool8;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;
typedef double float64;

typedef size_t memory_index;

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct THREAD_CONTEXT
{
    int Placeholder;
} THREAD_CONTEXT;

/* IMPORTANT(casey):

   These are NOT for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data!
*/

inline uint32
SafeTruncateUInt64(uint64 value)
{
    // TODO(casey): Defines for maximum values
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return result;
}

typedef struct DEBUG_READ_FILE_RESULT
{
    uint32 contents_size;
    void *contents;
} DEBUG_READ_FILE_RESULT;

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(THREAD_CONTEXT* thread, void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUG_READ_FILE_RESULT name(THREAD_CONTEXT* thread, char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(THREAD_CONTEXT* thread, char *filename, uint32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);


struct GAME_MEMORY
{
    bool32 is_initialized;

    uint64 permanent_storage_size;
    void* permanent_storage; // NOTE(casey): REQUIRED to be cleared to zero at startup

    uint64 transient_storage_size;
    void* transient_storage; // NOTE(casey): REQUIRED to be cleared to zero at startup

    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

struct GAME_AUDIO
{
    void *stream;
    uint32 size, depth, samples_per_tick;
};

struct GAME_BUTTON_STATE
{
    bool32 is_down;
    bool32 was_pressed;
    bool32 was_released;
    int transitions;
};


struct GAME_CONTROLLER_INPUT
{
    bool32 is_connected;

    union
    {
        GAME_BUTTON_STATE buttons[7];
        struct
        {
            GAME_BUTTON_STATE up;
            GAME_BUTTON_STATE right;
            GAME_BUTTON_STATE down;
            GAME_BUTTON_STATE left;
            GAME_BUTTON_STATE a;
            GAME_BUTTON_STATE b;
            GAME_BUTTON_STATE escape;
        };
    };
};

struct GAME_INPUT
{
    GAME_CONTROLLER_INPUT controllers[1];
    int key_buffer[256];
    int key_buffer_length;
    int key_buffer_position;

    int mouse_x;
    int mouse_y;
    GAME_BUTTON_STATE primary_click;

    GAME_BUTTON_STATE record_gif;
};

struct PIXEL_BACKBUFFER
{
    void *pixels;
    int h, w, pitch, depth;
};

#define GAME_UPDATE_AND_RENDER(name) int name(THREAD_CONTEXT* thread, GAME_MEMORY *memory, PIXEL_BACKBUFFER *render_buffer, GAME_INPUT *input, uint32 ticks )
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(THREAD_CONTEXT* thread, GAME_MEMORY *memory, GAME_AUDIO *audio, uint32 ticks)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);





#define PLATFORM_H
#endif
