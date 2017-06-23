#include "alpha_cube.h"

#include <windows.h>
#include <stdio.h>
#include <SDL.h>
#include <math.h>


#define GAME_COLOR_DEPTH 2
#define GAME_PITCH (GAME_WIDTH * GAME_COLOR_DEPTH / 8)

#define BOARD_X 88
#define BOARD_Y 40

#include "sound.cpp"
#include "random.h"

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
    { // from http://www.colourlovers.com/palette/4285766/evenin
        RGBA_COLOR* palette = state->palettes[i++];
        int j=0;
        palette[j++] = {0xf8, 0xf7, 0xd8, 0xFF};
        palette[j++] = {0xce, 0x89, 0x6a, 0xFF};
        palette[j++] = {0x78, 0x1c, 0x4d, 0xFF};
        palette[j++] = {0x1e, 0x03, 0x24, 0xFF};
    }
    { // from http://www.colourlovers.com/palette/3406503/aestrith
        RGBA_COLOR* palette = state->palettes[i++];
        int j=0;
        palette[j++] = {0x27, 0x16, 0x29, 0xFF};
        palette[j++] = {0x65, 0x4a, 0x66, 0xFF};
        palette[j++] = {0xf9, 0xf7, 0xe8, 0xFF};
        palette[j++] = {0xf9, 0xeb, 0x6d, 0xFF};
    }
    return 1;
}



//******************************************************************************
//**************************** BLOCKS ******************************************
//******************************************************************************

int GetBaseTilesForKind(int tiles[][2], int kind)
{
    if (kind == 0)
    {
        tiles[0][0] =  0; tiles[0][1] =  0;
        tiles[1][0] =  1; tiles[1][1] =  0;
        tiles[2][0] =  1; tiles[2][1] = -1;
        tiles[3][0] = -1; tiles[3][1] =  0;
        return 4;
    }
    if (kind == 1)
    {
        tiles[0][0] =  0; tiles[0][1] =  0;
        tiles[1][0] =  1; tiles[1][1] =  0;
        tiles[2][0] =  0; tiles[2][1] = -1;
        tiles[3][0] = -1; tiles[3][1] =  0;
        return 4;
    }
    if (kind == 2)
    {
        tiles[0][0] =  0; tiles[0][1] =  0;
        return 1;
    }
    Assert(false);
    return 0;
}

int GetBlockTiles(int tiles[][2], int rotate, int kind)
{
    int these_tiles[4][2];
    int length = GetBaseTilesForKind(these_tiles, kind);
    for (int i=0; i < 4; i++)
    {
        tiles[i][0] = (rotate % 2) ?  these_tiles[i][1] : these_tiles[i][0];
        tiles[i][1] = (rotate % 2) ? -these_tiles[i][0] : these_tiles[i][1];
        if (rotate >= 2)
        {
            tiles[i][0] *= -1;
            tiles[i][1] *= -1;
        }
    }
    return length;
}

TILE Board_GetTile(BOARD *board, int x, int y)
{
    TILE result = {};
    if (x >= 0 && x < board->width && y >= 0 && y < board->height)
    {
        result = board->tiles[(y * board->width) + x];
    }
    return result;
}

bool32 Board_TileExists(BOARD *board, int x, int y)
{
    return Board_GetTile(board, x, y).kind > 0;
}

void Board_SetTile(BOARD *board, int x, int y, TILE value)
{
    board->tiles[(y * board->width) + x] = value;
}

void Board_ClearTile(BOARD *board, int x, int y)
{
    board->tiles[(y * board->width) + x] = {};
}

void Board_Reset(BOARD *board)
{
    memset(board->tiles, 0, sizeof(TILE) * board->width * board->height);
    TILE tile = { 1 };
    // Board_SetTile(board, 12, 7, tile);
    // Board_SetTile(board, 13, 7, tile);
}

int GetBlockLanding(BOARD *board, GAME_BLOCK block)
{
    int col;
    int tiles[4][2];
    int length = GetBlockTiles(tiles, block.rotate, block.kind);

    for (col=block.y; col > 0; col--)
    {
        for (int i=0; i < length; i++)
        {
            int x = block.x + tiles[i][0];
            int y = col - 1 + tiles[i][1];
            if (x < 0 || x >= board->width || y < 0 || y >= board->height || Board_GetTile(board, x, y).kind)
            {
                goto END;
            }
        }
    }
    END: return col;
}

void ResetPosition(GAME_BLOCK* block, int kind)
{
    block->y = 18;
    block->x = 16;
    block->rotate = 0;
    block->kind = kind;
}

void PlaceBlockOnBoard(BOARD *board, GAME_BLOCK block)
{
    int tiles[4][2];
    int length = GetBlockTiles(tiles, block.rotate, block.kind);
    for (int i=0; i < length; i++)
    {
        TILE tile = {};
        tile.kind = block.kind + 1;
        if (tile.kind == 3) tile.on_fire = true;
        Board_SetTile(board, block.x + tiles[i][0], block.y + tiles[i][1], tile);
    }
}

void DropBlock(BOARD *board, GAME_BLOCK *block, int kind)
{
    GAME_BLOCK block_to_place = *block;
    block_to_place.y = GetBlockLanding(board, *block);
    PlaceBlockOnBoard(board, block_to_place);
    ResetPosition(block, kind);
}

void PlaceBlock(BOARD *board, GAME_BLOCK *block, int kind)
{
    PlaceBlockOnBoard(board, *block);
    ResetPosition(block, kind);
}



bool32 BlockIsValid(BOARD *board, GAME_BLOCK block)
{
    bool32 is_valid = true;
    int tiles[4][2];
    int length = GetBlockTiles(tiles, block.rotate, block.kind);
    for (int i=0; i < length; i++)
    {
        int x = block.x + tiles[i][0];
        int y = block.y + tiles[i][1];

        if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT || Board_GetTile(board, x, y).kind)
        {
            is_valid = false;
            break;
        }
    }
    return is_valid;
}

void RotateBlock(GAME_BLOCK *block, int dr)
{
    while (dr < 0)
    {
        dr += 4;
    }
    block->rotate = (block->rotate + dr) % 4;
}

bool32 BlockCanMoveHorizontally(BOARD *board, GAME_BLOCK block, int distance)
{
    while (distance > 0)
    {
        block.x++;
        distance--;
        if (!BlockIsValid(board, block)) return false;
    }
    while (distance < 0)
    {
        block.x--;
        distance++;
        if (!BlockIsValid(board, block)) return false;
    }
    return true;
}

bool32 BlockCanMoveVertically(BOARD *board, GAME_BLOCK block, int distance)
{
    while (distance > 0)
    {
        block.y++;
        distance --;
        if (!BlockIsValid(board, block)) return false;
    }
    return true;
}

bool32 BlockTryMoveHorizontally(BOARD *board, GAME_BLOCK *block, int distance)
{
    if (!BlockCanMoveHorizontally(board, *block, distance)) return false;
    block->x += distance;
    return true;
}

bool32 BlockTryMoveVertically(BOARD *board, GAME_BLOCK *block, int distance)
{
    if (!BlockCanMoveVertically(board, *block, distance)) return false;
    block->y += distance;
    return true;
}

bool32 BlockCanRotate(BOARD *board, GAME_BLOCK block, int dr)
{
    RotateBlock(&block, dr);
    return BlockIsValid(board, block);
}

bool32 BlockCanMove(BOARD *board, GAME_BLOCK block, int dx, int dy, int dr)
{
    return BlockTryMoveVertically(board, &block, dy) &&
           BlockTryMoveHorizontally(board, &block, dx) &&
           BlockCanRotate(board, block, dr);
}

bool32 BlockTryRotate(BOARD *board, GAME_BLOCK *block, int dr)
{
    if (!BlockCanRotate(board, *block, dr)) return false;
    RotateBlock(block, dr);
    return true;
}

bool32 BlockTryMove(BOARD *board, GAME_BLOCK *block, int dx, int dy, int dr)
{
    if (BlockCanMove(board, *block, dx, dy, dr))
    {
        block->x += dx;
        block->y += dy;
        RotateBlock(block, dr);
        return true;
    }
    return false;
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

void DrawTile(PIXEL_BACKBUFFER* buffer, int x, int y, TILE tile)
{
    int screen_x, screen_y;
    if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT)
    {
        screen_x = x * 8;
        screen_y = BOARD_Y + y * 8;
    }
    else
    {
        return;
    }
    if (tile.kind == 0)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 0);
    }
    if (tile.kind == 1)
    {
        DrawRect(buffer, screen_x + 1, screen_y + 1, 6, 6, 2);
        DrawRect(buffer, screen_x, screen_y, 1, 7, 0);
        DrawRect(buffer, screen_x, screen_y + 7, 7, 1, 0);
        DrawRect(buffer, screen_x + 1, screen_y, 7, 1, 3);
        DrawRect(buffer, screen_x + 7, screen_y + 1, 1, 7, 3);
    }
    if (tile.kind == 2)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 2);
        if (tile.on_fire) {
            DrawRect(buffer, screen_x + 2, screen_y + 2, 4, 4, 1);
        }
    }
    if (tile.kind == 3)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 3);
        DrawRect(buffer, screen_x + 1, screen_y + 1, 6, 6, 1);
    }
}

void DrawBlock(PIXEL_BACKBUFFER* buffer, GAME_BLOCK block, bool32 shadow)
{
    int tiles[4][2];
    int length = GetBlockTiles(tiles, block.rotate, block.kind);
    TILE tile = {};
    if (!shadow)
    {
        tile.kind = block.kind + 1;
    }
    for (int i=0; i < length; i++)
    {
        DrawTile(buffer, block.x + tiles[i][0], block.y + tiles[i][1], tile);
    }
}

void DrawCurrentBlock(PIXEL_BACKBUFFER* buffer, GAME_BLOCK block)
{
    DrawBlock(buffer, block, 0);
}

void DrawBlockLanding(PIXEL_BACKBUFFER* buffer, BOARD *board, GAME_BLOCK block)
{
    GAME_BLOCK landing_block = block;
    int landing = GetBlockLanding(board, block);
    if (landing != block.y)
    {
        landing_block.y = landing;
        DrawBlock(buffer, landing_block, 1);
    }
}

//******************************************************************************
//**************************** MAIN ********************************************
//******************************************************************************



GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    GAME_STATE *state;
    state = (GAME_STATE*) memory->permanent_storage;
    if (state != 0)
    {
        memset(audio->stream, 0, audio->size * audio->depth);
        GetSound(audio, state, ticks);
        memcpy(state->audio_buffer,audio->stream, 1024 * 4);
    }
}

bool32 Keypress(GAME_BUTTON_STATE *button)
{
    if (button->transitions >= 1)
    {
        button->transitions--;
        button->is_down = !button->is_down;
        if (button->is_down)
        {
            return true;
        }
    }
    return false;
}

int GetRandomKind(int *random_number_index)
{
    int index = *random_number_index;
    int random = RANDOM_NUMBER_TABLE[index] % 6;
    int result;
    if (index == 12) result = 2;
    else if (index % 2 == 0) result = 0;
    else result = 1;
    // if (random <= 2) result = 0;
    // else if (random <= 4) result = 1;
    // else result = 2;
    *random_number_index += 1;
    return result;
}

int8 CheckTileCanFall(BOARD *board, int8* tile_fall_map, int x, int y)
{
    int8 *tile_fall = tile_fall_map + (y * board->width) + x;
    if (*tile_fall == 0)
    {
        if (!Board_TileExists(board, x, y))
        {
            *tile_fall = 1;
        }
        else if (y == 0)
        {
            *tile_fall = -1;
        }
        else
        {
            *tile_fall = CheckTileCanFall(board, tile_fall_map, x, y - 1);
        }
    }
    return *tile_fall;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    GAME_STATE *state;
    PIXEL_BACKBUFFER *buffer;
    void* pixels;
    uint64 timer;
    int palette = 0;

    timer = StartProfiler();
    Assert(sizeof(GAME_STATE) <= memory->permanent_storage_size);
    state = (GAME_STATE*) memory->permanent_storage;
    buffer = &(state->buffer);


    if (!memory->is_initialized)
    {
        state->board.width = BOARD_WIDTH;
        state->board.height = BOARD_HEIGHT;
        memory_index used_memory = sizeof(GAME_STATE);
        InitializeArena(&state->board_arena, Kilobytes(16), (uint8 *) memory->permanent_storage + used_memory);
        used_memory += Kilobytes(16);
        state->board.tiles = PushArray(&state->board_arena, state->board.width * state->board.height, TILE);

        buffer->pixels = &state->pixels;
        buffer->pitch = 2 * GAME_WIDTH / 8;
        memory->is_initialized = true;
        ResetPosition(&state->block, GetRandomKind(&state->random_index));
        state->safety = 0;
        state->clock.bpm = 120.0f;
        state->clock.meter = 4;
        state->instrument.memory = (void*)&(state->instrument_state);
        state->instrument.Play = Instrument;
        InitColors(state);
    }

    int new_beats = state->clock.time.beats;
    int beats = max(new_beats - state->beats, 0);
    state->beats = new_beats;


    pixels = buffer->pixels;

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
        if (Keypress(&keyboard->move_right))
        {
            if (BlockTryMoveHorizontally(&state->board, &state->block, 1))
            {
                state->safety = 2;
            }
        }
        if (Keypress(&keyboard->move_left))
        {
            if (BlockTryMoveHorizontally(&state->board, &state->block, -1))
            {
                state->safety = 2;
            }
        }
        if (Keypress(&keyboard->rotate_clockwise))
        {
            if (BlockTryRotate(&state->board, &state->block, 1)
                || BlockTryMove(&state->board, &state->block,  1,  0, 1)
                || BlockTryMove(&state->board, &state->block,  0,  1, 1)
                || BlockTryMove(&state->board, &state->block,  2,  0, 1)
                || BlockTryMove(&state->board, &state->block,  0,  2, 1)
                || BlockTryMove(&state->board, &state->block, -1,  0, 1)
                || BlockTryMove(&state->board, &state->block,  0, -1, 1)
                || BlockTryMove(&state->board, &state->block, -2,  0, 1)
                || BlockTryMove(&state->board, &state->block,  0, -2, 1)
            )
            {
                state->safety = 2;
            }
        }
        if (Keypress(&keyboard->rotate_counterclockwise))
        {
            if (BlockTryRotate(&state->board, &state->block, -1)
                || BlockTryMove(&state->board, &state->block, -1,  0, -1)
                || BlockTryMove(&state->board, &state->block,  0,  1, -1)
                || BlockTryMove(&state->board, &state->block, -2,  0, -1)
                || BlockTryMove(&state->board, &state->block,  0,  2, -1)
                || BlockTryMove(&state->board, &state->block,  1,  0, -1)
                || BlockTryMove(&state->board, &state->block,  0, -1, -1)
                || BlockTryMove(&state->board, &state->block,  2,  0, -1)
                || BlockTryMove(&state->board, &state->block,  0, -2, -1)
            )
            {
                state->safety = 2;
            }
        }
        if (Keypress(&keyboard->drop))
        {
            int block_landing = GetBlockLanding(&state->board, state->block);
            DropBlock(&state->board, &state->block, GetRandomKind(&state->random_index));
        }
        if (Keypress(&keyboard->clear_board))
        {
            Board_Reset(&state->board);
        }
    }

    // BLOCK FALLS
    {
        while (beats--)
        {
            // state->clock.bpm += 1;
            if (state->block.y > 0 && state->block.y != GetBlockLanding(&state->board, state->block))
            {
                state->block.y--;
            }
            else if (state->safety)
            {
                state->safety--;
            }
            else
            {
                PlaceBlock(&state->board, &state->block, GetRandomKind(&state->random_index));
            }
        }
    }



    state->timer += ticks;
    int cycles = state->timer / 1000;
    state->timer %= 1000;
    while (cycles--)
    {
        // BURN DAMAGE
        {
            BOARD *board = &state->board;
            for (int y=0; y < board->height; y++)
            {
                for (int x=0; x < board->width; x++)
                {
                    TILE tile = Board_GetTile(board, x, y);
                    if (tile.on_fire)
                    {
                        tile.damage++;
                        if (tile.damage > 3) Board_ClearTile(board, x, y);
                        else Board_SetTile(board, x, y, tile);
                    }
                }
            }
        }

        // PROPAGATE FIRE
        {
            int to_burn_count = 0;
            BOARD *board = &state->board;
            for (int y = 0; y < board->height; y++)
            {
                for (int x = 0; x < board->width; x++)
                {
                    TILE tile = Board_GetTile(board, x, y);
                    if (tile.kind == 2 && !tile.on_fire)
                    {
                        if (Board_GetTile(board, x, y + 1).on_fire ||
                        Board_GetTile(board, x + 1, y).on_fire ||
                        Board_GetTile(board, x, y - 1).on_fire ||
                        Board_GetTile(board, x - 1, y).on_fire)
                        {
                            TILE_COORD *coord = PushStruct(&state->board_arena, TILE_COORD);
                            coord->x = x; coord->y = y;
                            to_burn_count++;
                        }
                    }
                }
            }
            while (to_burn_count > 0)
            {
                TILE_COORD *coord = PopStruct(&state->board_arena, TILE_COORD);
                TILE tile = Board_GetTile(board, coord->x, coord->y);
                tile.on_fire = true;
                Board_SetTile(board, coord->x, coord->y, tile);
                to_burn_count--;
            }
        }
    }

    // GRAVITY
    {
        state->gravity_timer += ticks;
        int cycles = state->gravity_timer / 100;
        state->gravity_timer %= 100;
        BOARD *board = &state->board;
        int8 *tile_fall_map = PushArray(&state->board_arena, board->height * board->width, int8);
        int8 *should_tile_fall = tile_fall_map;
        while (cycles--)
        {
            memset(tile_fall_map, 0, sizeof(int8) * board->height * board->width);
            for (int y = 0; y < board->height; y++)
            {
                for (int x = 0; x < board->width; x++)
                {
                    if (Board_TileExists(board, x, y))
                    {
                        CheckTileCanFall(board, tile_fall_map, x, y);
                    }
                }
            }
            for (int y = 0; y < board->height; y++)
            {
                for (int x = 0; x < board->width; x++)
                {
                    if (Board_TileExists(board, x, y) && tile_fall_map[(y * board->width) + x] == 1)
                    {
                        TILE tile = Board_GetTile(board, x, y);
                        Board_ClearTile(board, x, y);
                        Board_SetTile(board, x, y - 1, tile);
                    }
                }
            }
        }

        PopArray(&state->board_arena, board->height * board->width, int8);
    }


    // DRAW
    {
        DrawRect(buffer, 0, 0, GAME_WIDTH, GAME_HEIGHT, 1);
        DrawRect(buffer, 0, 0, GAME_WIDTH, BOARD_Y, 0);

        DrawBlockLanding(buffer, &state->board, state->block);
        DrawCurrentBlock(buffer, state->block);
        {
            for (int row=0; row < BOARD_HEIGHT; row++)
            {
                for (int col=0; col < BOARD_WIDTH; col++)
                {
                    TILE tile = Board_GetTile(&state->board, col, row);
                    if (tile.kind)
                    {
                        DrawTile(buffer, col, row, tile);
                    }
                }
            }
        }
    }

    // COPY ONTO BACKBUFFER
    {
        uint8* pixel_row;
        float horizontal_factor, vertical_factor;
        int left, right, top, bottom, scale_factor;

        horizontal_factor = (float) render_buffer->w / GAME_WIDTH;
        vertical_factor = (float) render_buffer->h / GAME_HEIGHT;
        scale_factor = (int)(min(horizontal_factor, vertical_factor));
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
