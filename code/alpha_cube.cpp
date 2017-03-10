#include "alpha_cube.h"


#include <windows.h>
#include <stdio.h>
#include <SDL.h>
#include <math.h>

#define GAME_HEIGHT 240
#define GAME_WIDTH 256
#define GAME_COLOR_DEPTH 2
#define GAME_PITCH (GAME_WIDTH * GAME_COLOR_DEPTH / 8)

#define BOARD_X 88
#define BOARD_Y 40

//******************************************************************************
//**************************** PROFILER ****************************************
//******************************************************************************

internal uint64
StartProfiler()
{
    return SDL_GetPerformanceCounter();
}

internal double
StopProfiler(uint64 start_time, bool32 print)
{
    uint64 end_time, time_difference, performance_frequency;
    double milliseconds;

    end_time = SDL_GetPerformanceCounter();
    time_difference = end_time - start_time;
    performance_frequency = SDL_GetPerformanceFrequency();
    milliseconds = (time_difference * 1000.0) / performance_frequency;
    // if (print)
    // {
    //     char FPSBuffer[256];
    //     snprintf(FPSBuffer, sizeof(FPSBuffer), "%.02f ms elapsed\n", milliseconds);
    //     OutputDebugStringA(FPSBuffer);
    // }

    return milliseconds;
}

//******************************************************************************
//**************************** UTILITIES ***************************************
//******************************************************************************

inline float Max(float a, float b)
{
    return a > b ? a : b;
}

inline float Min(float a, float b)
{
    return  a < b ? a : b;
}

inline int Min(int a, int b)
{
    return a < b ? a : b;
}

//******************************************************************************
//**************************** MEMORY ******************************************
//******************************************************************************
void* AllocatePermanentStorage(GAME_MEMORY* memory, int size)
{
    void* result;
    Assert(memory->permanent_storage_offset + size <= memory->permanent_storage_size);
    result = ((uint8*)memory->permanent_storage) + memory->permanent_storage_offset;
    memory->permanent_storage_offset += size;
    return result;
}

//******************************************************************************
//**************************** INIT ********************************************
//******************************************************************************

int InitColors(GAME_STATE* state)
{
    int i=0;
    {
        RGBA_COLOR* palette = state->palettes[i++];
        int j=0;
        palette[j++] = {0x9C, 0xBD, 0x0F, 0xFF};
        palette[j++] = {0x8C, 0xAD, 0x0F, 0xFF};
        palette[j++] = {0x30, 0x62, 0x30, 0xFF};
        palette[j++] = {0x0F, 0x38, 0x0F, 0xFF};
    }
    {
        RGBA_COLOR* palette = state->palettes[i++];
        int j=0;
        palette[j++] = {0xf8, 0xf7, 0xd8, 0xFF};
        palette[j++] = {0xce, 0x89, 0x6a, 0xFF};
        palette[j++] = {0x78, 0x1c, 0x4d, 0xFF};
        palette[j++] = {0x1e, 0x03, 0x24, 0xFF};
    }
    return 1;
}


//******************************************************************************
//**************************** BLOCKS ******************************************
//******************************************************************************

int GetBlockLanding(bool32 board[BOARD_HEIGHT][BOARD_WIDTH], int x, int y)
{
    int col;

    for (col=y; col > 0; col--)
    {
        if (board[col - 1][x])
        {
            break;
        }
    }
    return col;
}

//******************************************************************************
//**************************** DRAW ********************************************
//******************************************************************************

void DrawPoint(PIXEL_BACKBUFFER* buffer, int x, int y, uint8 color)
{
    uint8* pixel_quad = (uint8*) buffer->pixels + ((x + (y*GAME_WIDTH)) / 4);
    int position = (x + (y*GAME_WIDTH)) % 4;
    uint8 mask = 3 << position;
    *pixel_quad = (*pixel_quad & ~mask) | (color << (position * 2));
}

void DrawRect(PIXEL_BACKBUFFER* buffer, int x, int y, int w, int h, uint8 color)
{
    uint8* pixel_quad_row = (uint8*) buffer->pixels + ((x + (y*GAME_WIDTH)) / 4);
    for (int row=0; row < h; row++)
    {
        if (y + row >= 0 && y + row < GAME_HEIGHT)
        {
            uint8* pixel_quad = pixel_quad_row;
            int position ((x + ((y + row)*GAME_WIDTH)) % 4);
            for (int col=0; col < w; col++)
            {
                if (x + col >= 0 && x + col < GAME_WIDTH)
                {
                    uint8 mask = 3 << (position * 2);
                    *pixel_quad = (*pixel_quad & ~mask) | (color << (position * 2));
                    position++;
                    if (position == 4)
                    {
                        position = 0;
                        *pixel_quad++;
                    }
                }
            }
        }
        pixel_quad_row += buffer->pitch;
    }
}

void DrawTile(PIXEL_BACKBUFFER* buffer, int x, int y)
{
    int screen_x, screen_y;
    if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT)
    {
        screen_x = BOARD_X + x * 8;
        screen_y = BOARD_Y + y * 8;
    }
    else if (y == BOARD_HEIGHT)
    {
        screen_x = 124;
        screen_y = BOARD_HEIGHT*8 + 8 + BOARD_Y;
    }
    else
    {
        return;
    }
    DrawRect(buffer, screen_x + 1, screen_y + 1, 6, 6, 2);
    DrawRect(buffer, screen_x, screen_y, 1, 7, 0);
    DrawRect(buffer, screen_x, screen_y + 7, 7, 1, 0);
    DrawRect(buffer, screen_x + 1, screen_y, 7, 1, 3);
    DrawRect(buffer, screen_x + 7, screen_y + 1, 1, 7, 3);
}

void DrawCurrentBlock(PIXEL_BACKBUFFER* buffer, int x, int y, int landing)
{
    DrawTile(buffer, x, y);
    DrawRect(buffer, BOARD_X + (8*x), BOARD_Y + (8*landing), 8, 8, 0);
}

//******************************************************************************
//**************************** MAIN ********************************************
//******************************************************************************

void SinWave(GAME_AUDIO* audio, float32* stream, uint64 length, float frequency, float amplitude, void* memory)
{
    SIN_STATE* state = (SIN_STATE*) memory;
    uint32 samples_per_second = audio->samples_per_tick * 1000;
    float32 period = samples_per_second / frequency;
    float32 sample_frequency = frequency / samples_per_second;
    float32 step = sample_frequency * Tau32;
    for (uint32 i=0; i<length; i++)
    {
        *stream += ((float32)sin(state->x) * amplitude);
        stream++;
        state->x += step;
        while (state->x > Tau32)
        {
            state->x -= Tau32;
        }
    }
}

void Instrument(GAME_AUDIO* audio, float32* stream, uint64 length, float frequency, float amplitude, void* memory)
{
    INSTRUMENT_STATE* state = (INSTRUMENT_STATE*) memory;
    SinWave(audio, stream, length, frequency * 2.0f, amplitude * 0.1f, &(state->sins[0]));
    SinWave(audio, stream, length, frequency, amplitude * 0.5f, &(state->sins[1]));
}

// AUDIO_CLOCK ClockAdd(AUDIO_CLOCK clock, int meter, uint64 samples_per_beat, uint64 samples)
// {
//     AUDIO_CLOCK result = {};
//
//     result.samples = clock.samples + samples;
//     result.beat = clock.beat;
//     result.measure = clock.measure;
//     while (result.samples >= samples_per_beat)
//     {
//         result.samples -= samples_per_beat;
//         result.beat++;
//         if (result.beat >= meter)
//         {
//             result.beat -= meter;
//             result.measure++;
//         }
//     }
//     return result;
// }


// int Beat(AUDIO_CLOCK clock)
// {
//     if (clock.samples_per_beat == 0)
//     {
//         return 0;
//     }
//     else
//     {
//         return (int)(clock.samples / clock.samples_per_beat);
//     }
// }

// uint64 Samples(AUDIO_CLOCK clock, float32 beats)
// {
//     return (uint64)(beats * clock.samples_per_beat);
// }
//
// uint64 Samples(AUDIO_CLOCK clock, int measures, float32 beats)
// {
//     return Samples(clock, measures * clock.meter + beats);
// }

// TODO: Audio clock needs to be measured in beats, not samples, because we need
// to be able to change bpm.
void GetSound(GAME_AUDIO* audio, GAME_STATE* state, uint32 ticks)
{
    float32 *stream;
    AUDIO_CLOCK *clock = &(state->clock);

    float32 samples_per_minute = (float32)(audio->samples_per_tick * 1000 * 60);
    clock->samples_per_beat = samples_per_minute / clock->bpm;
    float64 elapsed = (ticks * audio->samples_per_tick) / clock->samples_per_beat;
    clock->time = clock->time.Plus(elapsed);
    audio->written -= elapsed;

    AUDIO_TIME cursor = clock->time.Plus(audio->written);

    {
        char text[256];
        snprintf(text, sizeof(text), "%d ticks elapsed\n", ticks);
        OutputDebugStringA(text);
    }

    uint64 written = 0;
    uint64 end = audio->size;
    stream = (float32*)audio->stream;
    AUDIO_TIME m = {};
    while(written < end)
    {
        m = m.Plus(1.0);
        if (written < end && cursor.LessThan(m)) {
            uint64 length = (uint64)min(m.Minus(cursor).Time() * clock->samples_per_beat, end - written);
            Instrument(audio, &stream[written], length, 220.0f, 0.5f, &(state->instrument));
            cursor = m;
            written += length;
        }
        m = m.Plus(1.0);
        if (written < end && cursor.LessThan(m)) {
            uint64 length = (uint64)min(m.Minus(cursor).Time() * clock->samples_per_beat, end - written);
            Instrument(audio, &stream[written], length, 440.0f, 0.5f, &(state->instrument));
            cursor = m;
            written += length;
        }
    }
    audio->written += audio->size / clock->samples_per_beat;
}

void PlaceBlock(bool32 board[BOARD_HEIGHT][BOARD_WIDTH], int block_x, int block_y)
{
    int row, col;

    board[block_y][block_x] = true;
    row = block_y;
    col = 0;
    while(col < BOARD_WIDTH && board[row][col])
    {
        col++;
    }
    if (col == BOARD_WIDTH)
    {
        for (col=0; col < BOARD_WIDTH; col++)
        {
            board[row][col] = false;
        }
        for (row = row ; row < BOARD_HEIGHT - 1; row++)
        {
            for (col=0; col < BOARD_WIDTH; col++)
            {
                board[row][col] = board[row+1][col];
            }
        }
        for (col=0; col < BOARD_WIDTH; col++)
        {
            board[row][col] = false;
        }
    }
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    GAME_STATE *state;
    state = (GAME_STATE*) memory->permanent_storage;
    if (state != 0)
    {
        memset(audio->stream, 0, audio->size * audio->depth);
        GetSound(audio, state, ticks);
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    GAME_STATE *state;
    PIXEL_BACKBUFFER *buffer;
    void* pixels;
    uint64 timer;
    int palette = 1;

    timer = StartProfiler();
    Assert(sizeof(GAME_STATE) <= memory->permanent_storage_size);
    state = (GAME_STATE*) memory->permanent_storage;
    buffer = &(state->buffer);


    if (!memory->is_initialized)
    {
        memory->permanent_storage_offset += sizeof(GAME_STATE);
        buffer->pixels = AllocatePermanentStorage(memory,  (2 * GAME_WIDTH * GAME_HEIGHT) / 8);
        buffer->pitch = 2 * GAME_WIDTH / 8;
        memory->is_initialized = true;
        state->block_x = 5;
        state->block_landing = GetBlockLanding(state->board, state->block_x, state->block_y);
        state->block_y = 19;
        state->is_moving = false;
        state->clock.bpm = 120.0f;
        state->clock.meter = 4;
        InitColors(state);
    }

    int new_beats = state->clock.time.beats;
    int beats = max(new_beats - state->beats, 0);
    state->beats = new_beats;

    pixels = buffer->pixels;

    // DRAW BACKGROUND
    DrawRect(buffer, 0, 0, GAME_WIDTH, GAME_HEIGHT, 3);
    DrawRect(buffer, BOARD_X - 1, BOARD_Y - 1, BOARD_WIDTH * 8 + 2, BOARD_HEIGHT * 8 + 2, 2);
    DrawRect(buffer, BOARD_X, BOARD_Y, BOARD_WIDTH * 8, BOARD_HEIGHT * 8, 1);

    {
        for (int row=0; row < BOARD_HEIGHT; row++)
        {
            for (int col=0; col < BOARD_WIDTH; col++)
            {
                if (state->board[row][col])
                {
                    DrawTile(buffer, col, row);
                }
            }
        }
    }

    // RESOLVE INPUT
    {
        GAME_CONTROLLER_INPUT *keyboard = &(input->controllers[0]);
        {
            GAME_BUTTON_STATE *button = &(keyboard->escape);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down)
                {
                    return 0;
                }
            }
        }
        {
            GAME_BUTTON_STATE *button = &(keyboard->move_right);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down && state->block_x < BOARD_WIDTH - 1 && !state->board[state->block_y][state->block_x + 1])
                {
                    state->block_x++;
                    state->block_landing = GetBlockLanding(state->board, state->block_x, state->block_y);
                    if (state->block_y >= 0 && state->board[state->block_y - 1][state->block_x])
                    {
                        state->is_moving = true;
                    }
                }
            }
        }
        {
            GAME_BUTTON_STATE *button = &(keyboard->move_left);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down && state->block_x > 0 && !state->board[state->block_y][state->block_x - 1])
                {
                    state->block_x--;
                    state->block_landing = GetBlockLanding(state->board, state->block_x, state->block_y);
                    if (state->block_y >= 0 && state->board[state->block_y - 1][state->block_x])
                    {
                        state->is_moving = true;
                    }
                }
            }
        }
        {
            GAME_BUTTON_STATE *button = &(keyboard->drop);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down)
                {
                    PlaceBlock(state->board, state->block_x, state->block_landing);
                    {
                        state->block_y = 19;
                        state->block_x = 5;
                        state->block_landing = GetBlockLanding(state->board, state->block_x, state->block_y);
                        state->clock.bpm += 1;
                    }
                }
            }
        }
        {
            GAME_BUTTON_STATE *button = &(keyboard->clear_board);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down)
                {
                    memset(state->board, 0, sizeof(state->board)[0][0] * BOARD_WIDTH * BOARD_HEIGHT);
                }
            }
        }
    }

    // BLOCKS FALL
    {
        while (beats--)
        {
            if (state->is_moving)
            {
                state->is_moving = false;
            }
            else if (state->block_y > 0 && !state->board[state->block_y - 1][state->block_x])
            {
                state->block_y--;
            }
            else
            {
                PlaceBlock(state->board, state->block_x, state->block_y);
                {
                    state->block_y = 19;
                    state->block_x = 5;
                    state->block_landing = GetBlockLanding(state->board, state->block_x, state->block_y);
                }
            }
        }
    }

    DrawCurrentBlock(buffer, state->block_x, state->block_y, state->block_landing);


    // DRAW ONTO BACKBUFFER
    {
        uint8* pixel_row;
        float horizontal_factor, vertical_factor;
        int left, right, top, bottom, scale_factor;

        horizontal_factor = (float) render_buffer->w / GAME_WIDTH;
        vertical_factor = (float) render_buffer->h / GAME_HEIGHT;
        scale_factor = (int)(Min(horizontal_factor, vertical_factor));
        left = (int) ((render_buffer->w - (GAME_WIDTH * scale_factor)) / 2.0f);
        right = (int) (left + (GAME_WIDTH * scale_factor));
        top = (int) ((render_buffer->h - (GAME_HEIGHT * scale_factor)) / 2.0f);
        bottom = (int) (top + (GAME_HEIGHT * scale_factor));

        pixel_row = (uint8*) render_buffer->pixels;
        uint8* pixel_quad = (uint8*) pixels;
        uint8 pixel_quad_value = *pixel_quad;
        int position = 0;

        RGBA_COLOR* render_pixels = (RGBA_COLOR*)render_buffer->pixels;

        int y = (GAME_HEIGHT - 1) * scale_factor + top;
        for (int row=0; row < GAME_HEIGHT; row++)
        {
            int x = left;
            for (int col=0; col < GAME_WIDTH; col++)
            {
                int color_index;
                RGBA_COLOR* render_row;
                RGBA_COLOR color;

                color_index = (pixel_quad_value) & 3;
                color = state->palettes[palette][color_index];
                render_row = render_pixels + (x + (y*render_buffer->w));
                for (int i=0; i < scale_factor; i++)
                {
                    RGBA_COLOR* render_pixel = render_row;
                    for (int j=0; j < scale_factor; j++)
                    {
                        *render_pixel++ = color;
                    }
                    render_row += render_buffer->w;
                }
                if (position < 3)
                {
                    pixel_quad_value = pixel_quad_value >> 2;
                    position++;
                }
                else
                {
                    position = 0;
                    *pixel_quad++;
                    pixel_quad_value = *pixel_quad;
                }
                x += scale_factor;
            }
            y -= scale_factor;
        }
    }
    StopProfiler(timer, true);
    return 1;
}
