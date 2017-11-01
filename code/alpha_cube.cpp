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
#include "block.cpp"
#include "block_graph.cpp"
#include "draw.cpp"

#define SPRITEMAP_SIZE 4096

PIXEL_BACKBUFFER DEBUGLoadSpritemap(THREAD_CONTEXT* thread, debug_platform_read_entire_file *readEntireFile, char* filename)
{
    PIXEL_BACKBUFFER result = {};
    DEBUG_READ_FILE_RESULT readResult = readEntireFile(thread, filename);
    if(readResult.contentsSize != 0)
    {
        result.pixels = readResult.contents;
    }
    result.w = 128;
    result.h = 128;
    result.pitch = 2 * result.w / 8;
    return result;
}

bool32 DEBUGSaveSpritemap(THREAD_CONTEXT* thread, debug_platform_write_entire_file* writeEntireFile, PIXEL_BACKBUFFER* spritemap)
{
    bool32 result = writeEntireFile(thread, "spritemap", spritemap->h * spritemap->pitch, spritemap->pixels);
    return result;
}



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


void GetPalette(RGBA_COLOR palette[4], int index)
{
    int i = 0;
    switch (index)
    {
        case 0:
        {
            int j=0;
            palette[j++] = {0x9C, 0xBD, 0x0F, 0xFF};
            palette[j++] = {0x8C, 0xAD, 0x0F, 0xFF};
            palette[j++] = {0x30, 0x62, 0x30, 0xFF};
            palette[j++] = {0x0F, 0x38, 0x0F, 0xFF};
        } break;
        case 1:
        {
            int j=0;
            palette[j++] = {0xf8, 0xf7, 0xd8, 0xFF};
            palette[j++] = {0xce, 0x89, 0x6a, 0xFF};
            palette[j++] = {0x78, 0x1c, 0x4d, 0xFF};
            palette[j++] = {0x1e, 0x03, 0x24, 0xFF};
        } break;
        case 2:
        {
            int j=0;
            palette[j++] = {0xf9, 0xf7, 0xe8, 0xFF};
            palette[j++] = {0xf9, 0xeb, 0x6d, 0xFF};
            palette[j++] = {0x65, 0x4a, 0x66, 0xFF};
            palette[j++] = {0x27, 0x16, 0x29, 0xFF};
        } break;
    }
}

//******************************************************************************
//**************************** MAIN ********************************************
//******************************************************************************


bool32 Keydown(GAME_BUTTON_STATE *button)
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

void Play(bool32 first, GAME_STATE *state, GAME_INPUT *input, uint32 ticks)
{
    bool32 reset = first;
    bool32 place_block = false;

    // RESOLVE INPUT
    {
        GAME_CONTROLLER_INPUT *keyboard = &(input->controllers[0]);
        if (keyboard->move_right.was_pressed)
        {
            if (BlockTryMoveHorizontally(&state->board, &state->board.block, 1))
            {
                state->safety = 2;
            }
        }
        if (keyboard->move_left.was_pressed)
        {
            if (BlockTryMoveHorizontally(&state->board, &state->board.block, -1))
            {
                state->safety = 2;
            }
        }
        if (keyboard->rotate_clockwise.was_pressed)
        {
            if (BlockTryRotate(&state->board, 1)
                || BlockTryMove(&state->board, { 1,  0}, 1)
                || BlockTryMove(&state->board, { 0,  1}, 1)
                || BlockTryMove(&state->board, { 2,  0}, 1)
                || BlockTryMove(&state->board, { 0,  2}, 1)
                || BlockTryMove(&state->board, {-1,  0}, 1)
                || BlockTryMove(&state->board, { 0, -1}, 1)
                || BlockTryMove(&state->board, {-2,  0}, 1)
                || BlockTryMove(&state->board, { 0, -2}, 1)
            )
            {
                state->safety = 2;
            }
        }
        if (keyboard->rotate_counterclockwise.was_pressed)
        {
            if (BlockTryRotate(&state->board, -1)
                || BlockTryMove(&state->board, {-1,  0}, -1)
                || BlockTryMove(&state->board, { 0,  1}, -1)
                || BlockTryMove(&state->board, {-2,  0}, -1)
                || BlockTryMove(&state->board, { 0,  2}, -1)
                || BlockTryMove(&state->board, { 1,  0}, -1)
                || BlockTryMove(&state->board, { 0, -1}, -1)
                || BlockTryMove(&state->board, { 2,  0}, -1)
                || BlockTryMove(&state->board, { 0, -2}, -1)
            )
            {
                state->safety = 2;
            }
        }
        if (keyboard->drop.was_pressed)
        {
            BOARD *board = &state->board;
            int block_landing = GetBlockLanding(board);

            board->block.pos.y = GetBlockLanding(board);
            place_block = true;
        }
        if (keyboard->clear_board.was_pressed)
        {
            reset = true;
        }
    }

    // RESET
    if (reset)
    {
        BOARD* board = &state->board;

        memset(board->tiles, 0, sizeof(TILE) * board->width * board->height);
        board->block_count = 1;

        state->random_number_index = 0;
        state->get_next_block = true;
    }

    // GET NEXT BLOCK
    if (state->get_next_block)
    {
        state->get_next_block = false;
        state->safety = 1;

        BOARD* board = &state->board;
        GAME_BLOCK *block = &board->block;
        int* random_number_index = &state->random_number_index;
        BLOCK_DEF* defs = state->block_defs;

        board->block_count++;
        int def = 1;
        if (*random_number_index > 0 && *random_number_index % 13 == 0)
        {
            def = 2;
        }
        // else if (*random_number_index % 8 == 0)
        // {
        //     def = 3;
        // }
        else if (*random_number_index % 2 == 1)
        {
            def = 2;
        }
        *random_number_index += 1;
        block->def = &defs[def];
        block->pos = TILE_COORD{18, 16};
        block->rotate = 0;
        block->kind = 0;
        for (int i=0; i < block->def->num_tiles; i++)
        {
            block->tiles[i].kind = block->def->tile_kind;
            block->tiles[i].on_fire = block->tiles[i].kind == Fire;
            block->tiles[i].health = 4;
            block->tiles[i].block = board->block_count;
            TILE_OFFSET offset = block->def->offsets[i];
        }
    }

    // BLOCK FALLS
#if 1
    {
        int new_beats = state->clock.time.beats;
        int beats = max(new_beats - state->beats, 0);
        state->beats = new_beats;

        while (beats--)
        {
            // state->clock.bpm += 1;
            if (state->safety)
            {
                state->safety--;
            }
            else if (state->board.block.pos.y > 0 && state->board.block.pos.y != GetBlockLanding(&state->board))
            {
                state->board.block.pos.y--;
            }
            else
            {
                place_block = true;
            }
        }
    }
#endif

    if (place_block)
    {
        BOARD* board = &state->board;

        TILE_COORD coords[4];
        GetTileTILE_COORDsForBlock(coords, board->block);
        for (int i=0; i < board->block.def->num_tiles; i++)
        {
            Board_SetTile(board, coords[i], board->block.tiles[i]);
        }
        state->get_next_block = true;
    }

    // FIRE
#if 1
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
                    TILE_COORD coord = {x, y};
                    TILE tile = Board_GetTile(board, coord);
                    if (tile.on_fire && tile.health > 0)
                    {
                        tile.health--;
                        Board_SetTile(board, coord, tile);
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
                    TILE_COORD coord = {x, y};
                    TILE tile = Board_GetTile(board, coord);
                    if (tile.kind == Wood && !tile.on_fire)
                    {
                        if (Board_GetTile(board, {x, y + 1}).on_fire ||
                            Board_GetTile(board, {x + 1, y}).on_fire ||
                            Board_GetTile(board, {x, y - 1}).on_fire ||
                            Board_GetTile(board, {x - 1, y}).on_fire)
                        {
                            TILE_COORD *tile_coord = PushStruct(&state->board_arena, TILE_COORD);
                            tile_coord->x = x; tile_coord->y = y;
                            to_burn_count++;
                        }
                    }
                }
            }
            while (to_burn_count > 0)
            {
                TILE_COORD coord = PopStruct(&state->board_arena, TILE_COORD);
                TILE tile = Board_GetTile(board, coord);
                tile.on_fire = true;
                Board_SetTile(board, coord, tile);
                to_burn_count--;
            }
        }
    }
#endif

    // GRAVITY
    if (state->gravity)
    {
        state->gravity_timer += ticks;
        int cycles = state->gravity_timer / 100;
        state->gravity_timer %= 100;
        BOARD *board = &state->board;
        MEMORY_ARENA *arena = &state->board_arena;
        while (cycles--)
        {
            BLOCK_GRAPH graph = {};
            graph.nodes = PushArray(arena, board->block_count, BLOCK_GRAPH_NODE);
            memset(graph.nodes, 0, sizeof(BLOCK_GRAPH_NODE) * board->block_count);
            graph.edges = (BLOCK_GRAPH_EDGE *)(arena->base + arena->used);
            // construct tile dependency graph
            for (int y=0; y < board->height; y++)
            {
                for (int x=0; x < board->width; x++)
                {
                    TILE_COORD coord = {x, y};
                    TILE tile = Board_GetTile(board, coord);
                    if (tile.kind)
                    {
                        int to = tile.block;
                        int from = 0;
                        bool32 insert = false;
                        if (coord.y == 0)
                        {
                            insert = true;
                        }
                        else
                        {
                            TILE below = Board_GetTile(board, {x, y - 1});
                            if (below.kind && below.block != tile.block)
                            {
                                insert = true;
                            }
                        }
                        if (insert)
                        {
                            BlockGraph_InsertEdge(&graph, arena, tile.block, 0);
                        }
                    }
                }
            }
            // sort graph
            for (int i=0; i < graph.edge_count; i++)
            {
                for (int j=i + 1; j < graph.edge_count; j++)
                {
                    if (
                        graph.edges[i].to > graph.edges[j].to ||
                        (
                            graph.edges[i].to == graph.edges[j].to &&
                            graph.edges[i].from > graph.edges[j].from
                        )
                    )
                    {
                        BLOCK_GRAPH_EDGE temp = graph.edges[i];
                        graph.edges[i] = graph.edges[j];
                        graph.edges[j] = temp;
                    }
                }
            }
            int block = 0;
            int count = 0;
            graph.nodes[0].edges = graph.edges;
            for (int i=0; i < graph.edge_count; i++)
            {
                if (graph.edges[i].to != block)
                {
                    graph.nodes[block].edge_count = count;
                    count = 0;
                    block = graph.edges[i].to;
                    graph.nodes[block].edges = &graph.edges[i];
                }
                count++;
            }
            graph.nodes[block].edge_count = count;
            BLOCK_GRAPH_NODE **stack = PushArray(arena, board->block_count, BLOCK_GRAPH_NODE *);
            int stack_size = 0;
            stack[stack_size++] = graph.nodes;
            while (stack_size > 0)
            {
                BLOCK_GRAPH_NODE *node = stack[--stack_size];
                node->discovered = true;
                for (int e = 0; e < node->edge_count; e++)
                {
                    BLOCK_GRAPH_NODE *other = &graph.nodes[node->edges[e].from];
                    if (!other->discovered)
                    {
                        stack[stack_size++] = other;
                    }
                }
            }
            for (int y = 0; y < board->height; y++)
            {
                for (int x = 0; x < board->width; x++)
                {
                    TILE_COORD coord = {x, y};
                    TILE tile = Board_GetTile(board, coord);
                    if (tile.kind && !graph.nodes[tile.block].discovered)
                    {
                        TILE tile = Board_GetTile(board, coord);
                        Board_ClearTile(board, coord);
                        Board_SetTile(board, {x, y - 1}, tile);
                    }
                }
            }
            PopArray(arena, board->block_count, BLOCK_GRAPH_NODE *);
            PopArray(arena, graph.edge_count, BLOCK_GRAPH_EDGE);
            PopArray(arena, board->block_count, BLOCK_GRAPH_NODE);
        }
    }

    // DESTROY BLOCKS
    {
        BOARD* board = &state->board;
        for (int y = 0; y < board->height; y++)
        {
            for (int x = 0; x < board->width; x++)
            {
                TILE_COORD coord = {x, y};
                TILE tile = Board_GetTile(board, coord);
                if (tile.kind && tile.health == 0)
                {
                    Board_ClearTile(board, coord);
                    // TODO split up the tiles into separate blocks where necessary
                }
            }
        }
    }

    // DRAW
    {
        PIXEL_BACKBUFFER *buffer = &state->buffer;
        BOARD *board = &state->board;

        DrawClear(buffer, 1);
        DrawRect(buffer, 0, 0, GAME_WIDTH, BOARD_Y, 0);

        DrawBlockLanding(buffer, board);
        DrawBlock(buffer, board->block);
        {
            for (int y=0; y < BOARD_HEIGHT; y++)
            {
                for (int x=0; x < BOARD_WIDTH; x++)
                {
                    TILE_COORD coord = {x, y};
                    bool32 connections[4];
                    TILE tile = Board_GetTile(board, coord);
                    connections[0] = Board_GetTile(board, {x, y + 1}).block == tile.block;
                    connections[1] = Board_GetTile(board, {x + 1, y}).block == tile.block;
                    connections[2] = Board_GetTile(board, {x, y - 1}).block == tile.block;
                    connections[3] = Board_GetTile(board, {x - 1, y}).block == tile.block;
                    if (tile.kind)
                    {
                        DrawTile(buffer, coord, tile, connections);
                    }
                }
            }
        }
    }
}

bool32 InRect(Coord pos, Rect rect)
{
    bool32 result = (pos.x > rect.x) && (pos.x <= rect.x + rect.w) && (pos.y > rect.y) && (pos.y <= rect.y + rect.h);
    return result;
}

bool32 InTile(Coord pos, TileCoord coord)
{
    bool32 result = InRect(pos, ToRect({coord.x, coord.y, 1, 1}));
    return result;
}

bool32 DebugButton(PIXEL_BACKBUFFER* buffer, GAME_BUTTON_STATE click, Coord mouse_pos, TileRect s_rect)
{
    bool32 result = 0;
    Rect rect = ToRect(s_rect);
    uint8 color = 2;
    if (InRect(mouse_pos, rect))
    {
        color = click.is_down ? 0 : 1;
        if (click.was_released)
        {
            result = 1;
        }
    }
    DrawRect(buffer, rect, color);
    return result;
}

void DrawText(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, char* string, TileRect rect, uint8 first_sprite, int cursor_sprite)
{
    char c;
    int i = 0;
    c = string[i++];
    int y = 0;
    int x = 0;
    while (c != 0 && ((c >= 'A' && c <= 'Z') || c == ' ' || c == '\n' || c == '.'))
    {
        if (c != ' ' && c != '\n')
        {
            uint8 sprite;
            if (c >= 'A' && c <= 'Z')
            {
                sprite = first_sprite + c - 'A';
            }
            else if (c == '.')
            {
                sprite = first_sprite + 26;
            }
            else sprite = 0;

            DrawSpriteAligned(buffer, spritemap, {rect.x + x, rect.y + y}, sprite);
        }
        if (c == '\n')
        {
            x = rect.w;
        }
        else
        {
            x++;
        }
        if (x >= rect.w)
        {
            x -= rect.w;
            y--;
        }
        if (y <= -rect.h)
        {
            break;
        }
        c = string[i++];
    }
    if (cursor_sprite >= 0 && cursor_sprite < 256)
    {
        DrawSpriteAligned(buffer, spritemap, {rect.x + x, rect.y + y}, (uint8)cursor_sprite);
    }
}

void DrawText(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, char* string, TileRect rect, uint8 first_sprite)
{
    DrawText(buffer, spritemap, string, rect, first_sprite, -1);
}

void DrawDebugBorder(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, TileRect rect, uint8 first_sprite)
{
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {rect.x, i + rect.y + 1};
        DrawSpriteAligned(buffer, spritemap, where, first_sprite);
    }
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {i + rect.x + 1, rect.y + rect.h - 1};
        DrawSpriteAligned(buffer, spritemap, where, first_sprite + 1);
    }
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {rect.x + rect.w - 1, i + rect.y + 1};
        DrawSpriteAligned(buffer, spritemap, where, first_sprite + 2);
    }
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {i + rect.x + 1, rect.y};
        DrawSpriteAligned(buffer, spritemap, where, first_sprite + 3);
    }
    DrawSpriteAligned(buffer, spritemap, {rect.x, rect.y + rect.h - 1}, first_sprite + 4);
    DrawSpriteAligned(buffer, spritemap, {rect.x + rect.w - 1, rect.y + rect.h - 1}, first_sprite + 5);
    DrawSpriteAligned(buffer, spritemap, {rect.x + rect.w - 1, rect.y}, first_sprite + 6);
    DrawSpriteAligned(buffer, spritemap, {rect.x, rect.y}, first_sprite + 7);
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    int selected_palette = 0;

    uint64 timer = StartProfiler();

    Assert(sizeof(GAME_STATE) <= memory->permanent_storage_size);
    GAME_STATE *state = (GAME_STATE*) memory->permanent_storage;


    // INITIALIZE
    bool32 first = 0;
    if (!memory->is_initialized)
    {
        first = 1;
        PIXEL_BACKBUFFER *buffer = &(state->buffer);
        
        state->board.width = BOARD_WIDTH;
        state->board.height = BOARD_HEIGHT;
        state->spritemap = DEBUGLoadSpritemap(thread, memory->DEBUGPlatformReadEntireFile, "spritemap");
        state->font_spritemap = DEBUGLoadSpritemap(thread, memory->DEBUGPlatformReadEntireFile, "font.sm");
        
        buffer->pixels = &state->pixels;
        buffer->w = GAME_WIDTH;
        buffer->h = GAME_HEIGHT;
        buffer->pitch = 2 * buffer->w / 8;
        memory->is_initialized = true;
        state->safety = 0;
        state->clock.bpm = 120.0f;
        state->clock.meter = 4;
        state->instrument.memory = (void*)&(state->instrument_state);
        state->instrument.Play = Instrument;
        // InitColors(state);
        state->gravity = true;
        state->mode = 1;

        memory_index used_memory = sizeof(GAME_STATE);
        InitializeArena(&state->board_arena, Kilobytes(32), (uint8 *) memory->permanent_storage + used_memory);
        used_memory += Kilobytes(32);
        state->board.tiles = PushArray(&state->board_arena, state->board.width * state->board.height, TILE);

        InitializeArena(&state->def_arena, Kilobytes(1), (uint8 *) memory->permanent_storage + used_memory);
        {
            state->board.block.tiles = PushArray(&state->def_arena, 4, TILE);
            state->block_defs = PushArray(&state->def_arena, 1, BLOCK_DEF);
            {
                int def_count = 0;
                {
                    BLOCK_DEF *block = &state->block_defs[def_count++];
                    block->tile_kind = Metal;
                    block->offsets[0].x =  0; block->offsets[0].y =  0;
                    block->offsets[1].x =  1; block->offsets[1].y =  0;
                    block->offsets[2].x =  1; block->offsets[2].y = -1;
                    block->offsets[3].x = -1; block->offsets[3].y =  0;
                    block->num_tiles = 4;
                }
                {
                    BLOCK_DEF *block = &state->block_defs[def_count++];
                    block->tile_kind = Wood;
                    block->offsets[0].x =  0; block->offsets[0].y =  0;
                    block->offsets[1].x =  1; block->offsets[1].y =  0;
                    block->offsets[2].x =  0; block->offsets[2].y = -1;
                    block->offsets[3].x = -1; block->offsets[3].y =  0;
                    block->num_tiles = 4;
                }
                {
                    BLOCK_DEF *block = &state->block_defs[def_count++];
                    block->tile_kind = Fire;
                    block->offsets[0].x =  0; block->offsets[0].y =  0;
                    block->num_tiles = 1;
                }
                {
                    BLOCK_DEF *block = &state->block_defs[def_count++];
                    block->tile_kind = Seed;
                    block->offsets[0].x =  0; block->offsets[0].y =  0;
                    block->num_tiles = 1;
                }
            }
        }
    }

    {
        GAME_CONTROLLER_INPUT *keyboard = &(input->controllers[0]);
        for (int i=0; i < ArrayCount(keyboard->buttons); i++)
        {
            GAME_BUTTON_STATE *button = &(keyboard->buttons[i]);
            button->was_pressed = 0;
            button->was_released = 0;
            if (button->transitions > 0)
            {
                button->transitions--;
                button->is_down = !button->is_down;
                if (button->is_down) button->was_pressed = 1;
                else button->was_released = 1;
            }
        }
    }

    int key = -1;
    if (input->key_buffer_length > 0)
    {
        key = input->key_buffer[input->key_buffer_position];
        input->key_buffer_position++;
        input->key_buffer_position %= 256;
        input->key_buffer_length--;
    }

    if (key == '\t')
    {
        state->mode = (state->mode == 1) ? 2 : 1;
    }

    if (state->mode == 0)
    {
        Play(first, state, input, ticks);
    }
    else if (state->mode == 1)
    {
        if (key >= '1' && key <= '8')
        {
            int selection = key - '0' - 1;
            if (selection < state->selected_sprites_count)
            {
                state->current_sprite = (uint8)selection;
            }
        }
        else if (key == ' ')
        {
            state->edit_mode = (state->edit_mode == 0) ? 1 : 0;
        }
        else if (key == 's')
        {
            DEBUGSaveSpritemap(thread, memory->DEBUGPlatformWriteEntireFile, &state->spritemap);
        }
        else if (key == 'q')
        {
            state->cursor_color = 0;
        }
        else if (key == 'w')
        {
            state->cursor_color = 1;
        }
        else if (key == 'e')
        {
            state->cursor_color = 2;
        }
        else if (key == 'r')
        {
            state->cursor_color = 3;
        }


        state->selected_sprites_count = 8;
        GAME_BUTTON_STATE click = input->controllers[0].primary_click;
        Coord pos;
        {
            Coord external_pos = input->controllers[0].mouse_position;
            float x_scale = ((float)state->buffer.w) / render_buffer->w;
            float y_scale = ((float)state->buffer.h) / render_buffer->h;
            pos.x = (int)round(external_pos.x * x_scale); 
            pos.y = (int)round(external_pos.y * y_scale); 
        }
        if (state->edit_mode == 0)
        {
            DrawClear(&state->buffer, 3);
            DrawDebugBorder(&state->buffer, &state->spritemap, {7, 4, 18, 18}, 16);
            for (uint8 row=0; row < 16; row++)
            {
                for (uint8 col=0; col < 16; col++)
                {
                    uint8 tile = row * 16 + col;
                    TileCoord where = {col + 8, row + 5};
                    DrawSpriteAligned(&state->buffer, &state->spritemap, where, tile);
                    if (InTile(pos, where) && click.was_released)
                    {
                        state->selected_sprites[state->current_sprite] = tile;
                    }
                }
            }

            for (int i=0; i < 8; i++)
            {
                int x = i * 2 + 8;
                if (i < state->selected_sprites_count)
                {
                    DrawSpriteAligned(&state->buffer, &state->spritemap, {x, 22}, state->selected_sprites[i]);
                    uint8 bubble = (state->current_sprite == i) ? 3 : 2;
                    DrawSpriteAligned(&state->buffer, &state->spritemap, {x, 23}, bubble);
                }
                else if (i == state->selected_sprites_count)
                {
                    TileCoord where = {x, 22};
                    DrawSpriteAligned(&state->buffer, &state->spritemap, where, 4);
                    if (InTile(pos, where) && click.was_released && state->selected_sprites_count < 8)
                    {
                        state->selected_sprites[state->selected_sprites_count] = 0;
                        state->selected_sprites_count++;
                    }
                }
            }

            if (DebugButton(&state->buffer, click, pos, {15, 2, 2, 1}))
            {
                state->edit_mode = 1;
            }

        }
        else
        {
            if (key == 'c')
            {
                for (int x=0; x < 8; x++)
                {
                    for (int y=0; y < 8; y++)
                    {
                        DrawPointInSprite(&state->spritemap, {x, y}, state->selected_sprites[state->current_sprite], state->cursor_color);
                    }
                }
            }
            DrawClear(&state->buffer, 3);
            DrawDebugBorder(&state->buffer, &state->spritemap, {3, 2, 26, 26}, 16);
            
            for (uint8 color=0; color < 4; color++)
            {
                Rect rect = {256 - 8 * 3, 8 * 3 + 8 * 3 * 2 * color, 8 * 3, 8 * 3 * 2};
                DrawRect(&state->buffer, rect, color);
                if (InRect(pos, rect) && click.was_released)
                {
                    state->cursor_color = color;
                }
            }

            for (int i=0; i < 8; i++)
            {
                int y = 25 - (i * 2);
                if (i < state->selected_sprites_count)
                {
                    DrawSpriteAligned(&state->buffer, &state->spritemap, {1, y}, state->selected_sprites[i]);
                    uint8 bubble = (state->current_sprite == i) ? 3 : 2;
                    DrawSpriteAligned(&state->buffer, &state->spritemap, {2, y}, bubble);
                }
            }

            for (int r=0; r < 8; r++)
            {
                for (int c=0; c < 8; c++)
                {
                    uint8 color = GetPointFromSprite(&state->spritemap, {c, r}, state->selected_sprites[state->current_sprite]);
                    bool32 hover = 0;
                    Rect rect = {8 * 4 + 8 * 3 * c, 8 * 3 + 8 * 3 * r, 8 * 3, 8 * 3};
                    if (InRect(pos, rect))
                    {
                        if (click.is_down)
                        {
                            color = state->cursor_color;
                            DrawPointInSprite(&state->spritemap, {c, r}, state->selected_sprites[state->current_sprite], color);
                        }
                        hover = 1;
                    }
                    DrawRect(&state->buffer, rect, color);
                    if (hover)
                    {
                        DrawRect(&state->buffer, {rect.x + 8, rect.y + 8, 8, 8}, state->cursor_color);
                    } 
                }
    
            }
            if (DebugButton(&state->buffer, click, pos, {24, 1, 2, 1}))
            {
                DEBUGSaveSpritemap(thread, memory->DEBUGPlatformWriteEntireFile, &state->spritemap);
            }
            if (DebugButton(&state->buffer, click, pos, {1, 27, 2, 2}))
            {
                state->edit_mode = 0;
            }
    
        }
        {
            uint8 sprite = click.is_down ? 1 : 0;
            DrawSprite(&state->buffer, &state->spritemap, {pos.x, pos.y - 8}, sprite, 0);
            // uint8 color = click.is_down ? 2 : 3;
            // DrawRect(&state->buffer, pos.x, pos.y - 4, 4, 4, color);
        }

        // DrawText(&state->buffer, &state->spritemap, "HERE IS A SENTENCE.\nAND HERE IS ANOTHER.", {0, 29, 32, 4}, 32);
    }
    else
    {
        DrawClear(&state->buffer, 3);
        char c = 0;
        if (key >= 'a' && key <= 'z')
        {
            c = (char)key - ('a' - 'A');
        }
        else if (key == ' ' || key == '.')
        {
            c = (char)key;
        }
        else if (key == '\n' || key == '\r')
        {
            c = '\n';
        }
        if (key == 8 && state->editor_text_position > 0)
        {
            state->editor_text[--state->editor_text_position] = 0; 
        }
        else if (c > 0)
        {
            state->editor_text[state->editor_text_position++] = c;
        }
        DrawText(&state->buffer, &state->spritemap, state->editor_text, {1, 28, 30, 28}, 32, 5);
    }

    // COPY ONTO BACKBUFFER
    {
        PIXEL_BACKBUFFER *buffer = &state->buffer;
        void* pixels = buffer->pixels;

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

        RGBA_COLOR palette[4];
        GetPalette(palette, selected_palette);

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
                color = palette[color_index];
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
