#if !defined(ALPHA_CUBE_H)

#include "platform.h"
#include "sound.h"

#define BOARD_WIDTH 32
#define BOARD_HEIGHT 20

struct RGBA_COLOR
{
    uint8 r,g,b,a;
};

struct GAME_BLOCK
{
    int x, y, rotate;
};

struct GAME_STATE
{
    RGBA_COLOR palettes[3][4];
    PIXEL_BACKBUFFER buffer;
    bool32 board[BOARD_HEIGHT][BOARD_WIDTH];
    int frames;
    GAME_BLOCK block;
    int safety;
    INSTRUMENT_STATE instrument_state;
    INSTRUMENT instrument;
    AUDIO_CLOCK clock;
    int beats;
    float32 audio_buffer[1024];
};

#define ALPHA_CUBE_H
#endif
