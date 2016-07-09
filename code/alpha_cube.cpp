#include "alpha_cube.h"


#include <windows.h>
#include <stdio.h>
#include <SDL.h>

#define GAME_HEIGHT 240
#define GAME_WIDTH 256
#define GAME_COLOR_DEPTH 2
#define GAME_PITCH (GAME_WIDTH * GAME_COLOR_DEPTH / 8)

#define BOARD_X 88
#define BOARD_Y 40
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

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
    if (print)
    {
        char FPSBuffer[256];
        snprintf(FPSBuffer, sizeof(FPSBuffer), "%.02f ms elapsed\n", milliseconds);
        OutputDebugStringA(FPSBuffer);
    }

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

int InitColors(GAME_STATE* game_state)
{
    RGBA_COLOR* palette = game_state->palettes[0];
    palette[0] = {0x9C, 0xBD, 0x0F, 0xFF};
    palette[1] = {0x8C, 0xAD, 0x0F, 0xFF};
    palette[2] = {0x30, 0x62, 0x30, 0xFF};
    palette[3] = {0x0F, 0x38, 0x0F, 0xFF};
    return 1;
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
    if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT)
    {
        DrawRect(buffer, BOARD_X + x * 8, BOARD_Y + y * 8, 8, 8, 2);
    }
    if (y == BOARD_HEIGHT)
    {
        DrawRect(buffer, 124, BOARD_HEIGHT*8 + 8 + BOARD_Y, 8, 8, 2);
    }
}

//******************************************************************************
//**************************** MAIN ********************************************
//******************************************************************************

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    GAME_STATE *game_state;
    PIXEL_BACKBUFFER *buffer;
    void* pixels;

    uint64 timer = StartProfiler();
    Assert(sizeof(GAME_STATE) <= memory->permanent_storage_size);
    game_state = (GAME_STATE*) memory->permanent_storage;
    buffer = &(game_state->buffer);


    if (!memory->is_initialized)
    {
        memory->permanent_storage_offset += sizeof(GAME_STATE);
        buffer->pixels = AllocatePermanentStorage(memory,  (2 * GAME_WIDTH * GAME_HEIGHT) / 8);
        buffer->pitch = 2 * GAME_WIDTH / 8;
        memory->is_initialized = true;
        game_state->block_x = 5;
        game_state->block_y = 19;
        game_state->fall_speed = 60.0f;
        game_state->fall_time = 0.0f;
        InitColors(game_state);
    }


    pixels = buffer->pixels;

    // DRAW BACKGROUND
    DrawRect(buffer, 0, 0, GAME_WIDTH, GAME_HEIGHT, 3);
    DrawRect(buffer, BOARD_X, BOARD_Y, BOARD_WIDTH * 8, BOARD_HEIGHT * 8, 1);

    {
        for (int row=0; row < BOARD_HEIGHT; row++)
        {
            for (int col=0; col < BOARD_WIDTH; col++)
            {
                if (game_state->board[row][col])
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
            GAME_BUTTON_STATE *button = &(keyboard->move_right);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down && game_state->block_x < BOARD_WIDTH - 1)
                {
                    game_state->block_x++;
                }
            }
        }
        {
            GAME_BUTTON_STATE *button = &(keyboard->move_left);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down && game_state->block_x > 0)
                {
                    game_state->block_x--;
                }
            }
        }
        {
            GAME_BUTTON_STATE *button = &(keyboard->drop);
            if (button->transitions >= 1)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                game_state->fall_quickly = button->is_down;
            }
        }
    }

    // BLOCKS FALL
    {
        float fall_speed = game_state->fall_speed;
        if (game_state->fall_quickly)
        {
            fall_speed = Min(1.2f, fall_speed / 2.0f);
            while (game_state->fall_time > fall_speed * 2.0f)
            {
                game_state->fall_time -= fall_speed;
            }
        }
        game_state->fall_time += 1.0f;
        if (game_state->fall_time >= fall_speed)
        {
            if (game_state->block_y > 0)
            {
                game_state->block_y--;
            }
            else if (game_state->block_y == 0)
            {
                game_state->board[game_state->block_y][game_state->block_x] = true;
                game_state->block_y = 20;
                game_state->block_x = 5;
            }
            game_state->fall_time -= fall_speed;
        }
    }


    DrawTile(buffer, game_state->block_x, game_state->block_y);


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
                color = game_state->palettes[0][color_index];
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

}
