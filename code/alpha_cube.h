#if !defined(ALPHA_CUBE_H)

#include "platform.h"
#include "sound.h"

struct RGBA_COLOR
{
    uint8 r,g,b,a;
};

struct GAME_STATE
{
    RGBA_COLOR palettes[2][4];
    PIXEL_BACKBUFFER buffer;
    bool32 board[BOARD_HEIGHT][BOARD_WIDTH];
    int frames;
    int block_x;
    int block_y;
    int block_landing;
    bool32 is_moving;
    INSTRUMENT_STATE instrument_state;
    INSTRUMENT instrument;
    AUDIO_CLOCK clock;
    int beats;
    float32 audio_buffer[1024];
};

#define ALPHA_CUBE_H
#endif
