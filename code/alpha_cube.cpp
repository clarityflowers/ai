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
#include "board.cpp"

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

void RotateTileOffsets(TILE_OFFSET out[4], TILE_OFFSET in[4], int count, int amount)
{
    amount *= -1;
    while (amount < 0) amount += 4;

    int8 sin = (int8)((amount % 4) - 2) % 2;
    int8 cos = (int8)(((amount + 3) % 4) - 2) %2;
    for (int i=0; i < count; i++)
    {
        int8 x = in[i].x;
        int8 y = in[i].y;
        out[i].x = (cos * x) + (sin * y);
        out[i].y = (-sin * x) + (cos * y);
    }
}

//******************************************************************************
//**************************** BLOCKS ******************************************
//******************************************************************************

void GetTileCoordsForBlock(TILE_COORD coords[4], GAME_BLOCK block)
{
    TILE_OFFSET offsets[4];
    RotateTileOffsets(offsets, block.def->offsets, block.def->num_tiles, block.rotate);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        coords[i].x = offsets[i].x + block.x;
        coords[i].y = offsets[i].y + block.y;
    }
}

int GetBlockLanding(BOARD *board, GAME_BLOCK block)
{
    TILE_COORD coords[4];
    GetTileCoordsForBlock(coords, block);
    int step_down;
    for (step_down = 0; step_down < block.y; step_down++)
    {
        for (int i=0; i < block.def->num_tiles; i++)
        {
            int x = coords[i].x;
            int y = coords[i].y - step_down - 1;
            if (x < 0 || x >= board->width || y < 0 || y >= board->height || Board_TileExists(board, x, y))
            {
                goto END;
            }
        }
    }
    END:
    int result = block.y - step_down;
    return result;
}


void PlaceBlockOnBoard(BOARD *board, GAME_BLOCK block)
{
    TILE_COORD coords[4];
    GetTileCoordsForBlock(coords, block);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        Board_SetTile(board, coords[i].x, coords[i].y, block.tiles[i]);
    }
}

void Block_GetNext(GAME_BLOCK *block, BLOCK_DEF* defs, int *random_number_index)
{
    int def = 0;
    // if (*random_number_index % 2 == 1)
    // {
    //     def = 1;
    // }
    *random_number_index += 1;
    block->def = &defs[def];
    block->y = 18;
    block->x = 16;
    block->rotate = 0;
    block->kind = 0;
    for (int i=0; i < block->def->num_tiles; i++)
    {
        block->tiles[i].kind = block->def->tile_kind;
        block->tiles[i].on_fire = false;
        block->tiles[i].health = false;
        TILE_OFFSET offset = block->def->offsets[i];
        for (int c = 0; c < 4; c++)
        {
            block->tiles[i].connected[c] = false;
        }
        for (int j=0; j < block->def->num_tiles; j++)
        {
            if (j != i)
            {
                TILE_OFFSET other = block->def->offsets[j];
                if (other.x == offset.x && other.y == offset.y + 1)
                {
                    block->tiles[i].connected[0] = true;
                }
                if (other.y == offset.y && other.x == offset.x + 1)
                {
                    block->tiles[i].connected[1] = true;
                }
                if (other.x == offset.x && other.y == offset.y - 1)
                {
                    block->tiles[i].connected[2] = true;
                }
                if (other.y == offset.y && other.x == offset.x - 1)
                {
                    block->tiles[i].connected[3] = true;
                }
            }
        }
    }
}

void DropBlock(BOARD *board, GAME_BLOCK *block, BLOCK_DEF *defs, int *random_number_index)
{
    GAME_BLOCK block_to_place = *block;
    block_to_place.y = GetBlockLanding(board, *block);
    PlaceBlockOnBoard(board, block_to_place);
    Block_GetNext(block, defs, random_number_index);
}

void PlaceBlock(BOARD *board, GAME_BLOCK *block, BLOCK_DEF *defs, int *random_number_index)
{
    PlaceBlockOnBoard(board, *block);
    Block_GetNext(block, defs, random_number_index);
}



bool32 BlockIsValid(BOARD *board, GAME_BLOCK block)
{
    bool32 is_valid = true;
    TILE_COORD coords[4];
    GetTileCoordsForBlock(coords, block);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        int x = coords[i].x;
        int y = coords[i].y;

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

void RotateBlockTiles(GAME_BLOCK *block, int dr)
{
    while (dr < 0)
    {
        dr += 4;
    }
    RotateBlock(block, dr);
    for (int i = 0; i < block->def->num_tiles; i++)
    {
        bool32 connected[4];
        for (int c = 0; c < 4; c++)
        {
            connected[c] = block->tiles[i].connected[c];
        }
        for (int c = 0; c < 4; c++)
        {
            block->tiles[i].connected[c] = connected[(c - dr + 4) % 4];
        }
    }
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
    RotateBlockTiles(block, dr);
    return true;
}

bool32 BlockTryMove(BOARD *board, GAME_BLOCK *block, int dx, int dy, int dr)
{
    if (BlockCanMove(board, *block, dx, dy, dr))
    {
        block->x += dx;
        block->y += dy;
        RotateBlockTiles(block, dr);
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
    uint8 mask = 3 << (position * 2);
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

void DrawTile(PIXEL_BACKBUFFER* buffer, int x, int y, TILE tile, bool32 shadow = false)
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
    if (shadow)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 0);
    }
    else if (tile.kind == Metal)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 2);

        DrawPoint(buffer, screen_x, screen_y + 7, 0);
        DrawPoint(buffer, screen_x + 7, screen_y, 3);
        DrawPoint(buffer, screen_x, screen_y, 0);
        DrawPoint(buffer, screen_x + 7, screen_y + 7, 3);

        if (!tile.connected[0])
        {
            DrawRect(buffer, screen_x + 1, screen_y + 7, 6, 1, 0);
            if (tile.connected[1])
            {
                DrawPoint(buffer, screen_x + 7, screen_y + 7, 0);
            }
        }
        if (!tile.connected[1])
        {
            DrawRect(buffer, screen_x + 7, screen_y + 1, 1, 6, 3);
        }
        if (!tile.connected[2])
        {
            DrawRect(buffer, screen_x + 1, screen_y, 6, 1, 3);
        }
        if (tile.connected[3])
        {
            DrawPoint(buffer, screen_x, screen_y, 3);
        }
        else
        {
            DrawRect(buffer, screen_x, screen_y + 1, 1, 6, 0);
        }
    }
    else if (tile.kind == Wood)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 2);
        if (tile.on_fire) {
            DrawRect(buffer, screen_x + 2, screen_y + 2, 4, 4, 1);
        }
    }
    else if (tile.kind == Wood)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 3);
        DrawRect(buffer, screen_x + 1, screen_y + 1, 6, 6, 1);
    }
}

void DrawBlock(PIXEL_BACKBUFFER* buffer, GAME_BLOCK block, bool32 shadow)
{
    TILE_COORD coords[4];
    GetTileCoordsForBlock(coords, block);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        DrawTile(buffer, coords[i].x, coords[i].y, block.tiles[i], shadow);
    }
}

void DrawCurrentBlock(PIXEL_BACKBUFFER* buffer, GAME_BLOCK block)
{
    DrawBlock(buffer, block, false);
}

void DrawBlockLanding(PIXEL_BACKBUFFER* buffer, BOARD *board, GAME_BLOCK block)
{
    GAME_BLOCK landing_block = block;
    int landing = GetBlockLanding(board, block);
    if (landing != block.y)
    {
        landing_block.y = landing;
        DrawBlock(buffer, landing_block, true);
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
        InitializeArena(&state->board_arena, Kilobytes(32), (uint8 *) memory->permanent_storage + used_memory);
        used_memory += Kilobytes(32);
        state->board.tiles = PushArray(&state->board_arena, state->board.width * state->board.height, TILE);

        InitializeArena(&state->def_arena, Kilobytes(1), (uint8 *) memory->permanent_storage + used_memory);
        {
            state->block.tiles = PushArray(&state->def_arena, 4, TILE);
            state->block_defs = PushArray(&state->def_arena, 1, BLOCK_DEF);
            {
                int def_count = 0;
                {
                    BLOCK_DEF *block = &state->block_defs[def_count++];
                    block->tile_kind = Metal;
                    block->offsets[0].x =  0; block->offsets[0].y =  0;
                    block->offsets[1].x =  1; block->offsets[1].y =  0;
                    block->offsets[2].x =  0; block->offsets[2].y = -1;
                    block->offsets[3].x = -1; block->offsets[3].y =  0;
                    block->num_tiles = 4;
                }
                {
                    BLOCK_DEF *block = &state->block_defs[def_count++];
                    block->tile_kind = Wood;
                    block->offsets[0].x =  0; block->offsets[0].y =  0;
                    block->offsets[1].x =  1; block->offsets[1].y =  0;
                    block->offsets[2].x =  1; block->offsets[2].y = -1;
                    block->offsets[3].x = -1; block->offsets[3].y =  0;
                    block->num_tiles = 4;
                }
            }
        }

        buffer->pixels = &state->pixels;
        buffer->pitch = 2 * GAME_WIDTH / 8;
        memory->is_initialized = true;
        Block_GetNext(&state->block, state->block_defs, &state->random_number_index);
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
            DropBlock(&state->board, &state->block, state->block_defs, &state->random_number_index);
        }
        if (Keypress(&keyboard->clear_board))
        {
            Board_Reset(&state->board);
        }
    }

#if 0
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
                PlaceBlock(&state->board, &state->block, state->block_defs, &state->random_number_index);
            }
        }
    }
#endif

#if 0
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
#endif

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
