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

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    int palette = 0;

    uint64 timer = StartProfiler();

    Assert(sizeof(GAME_STATE) <= memory->permanent_storage_size);
    GAME_STATE *state = (GAME_STATE*) memory->permanent_storage;

    bool32 reset = false;
    bool32 place_block = false;

    // INITIALIZE
    if (!memory->is_initialized)
    {
        PIXEL_BACKBUFFER *buffer = &(state->buffer);

        state->board.width = BOARD_WIDTH;
        state->board.height = BOARD_HEIGHT;
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

        buffer->pixels = &state->pixels;
        buffer->pitch = 2 * GAME_WIDTH / 8;
        memory->is_initialized = true;
        state->safety = 0;
        state->clock.bpm = 120.0f;
        state->clock.meter = 4;
        state->instrument.memory = (void*)&(state->instrument_state);
        state->instrument.Play = Instrument;
        InitColors(state);
        state->gravity = true;

        reset = true;
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
        if (Keydown(&keyboard->move_right))
        {
            if (BlockTryMoveHorizontally(&state->board, &state->board.block, 1))
            {
                state->safety = 2;
            }
        }
        if (Keydown(&keyboard->move_left))
        {
            if (BlockTryMoveHorizontally(&state->board, &state->board.block, -1))
            {
                state->safety = 2;
            }
        }
        if (Keydown(&keyboard->rotate_clockwise))
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
        if (Keydown(&keyboard->rotate_counterclockwise))
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
        if (Keydown(&keyboard->drop))
        {
            BOARD *board = &state->board;
            int block_landing = GetBlockLanding(board);

            board->block.pos.y = GetBlockLanding(board);
            place_block = true;
        }
        if (Keydown(&keyboard->clear_board))
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
        block->pos.y = 18;
        block->pos.x = 16;
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
                    if (tile.on_fire)
                    {
                        tile.health--;
                        if (tile.health == 0) Board_ClearTile(board, coord);
                        else Board_SetTile(board, coord, tile);
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

    // DRAW
    {
        PIXEL_BACKBUFFER *buffer = &state->buffer;
        BOARD *board = &state->board;

        DrawRect(buffer, 0, 0, GAME_WIDTH, GAME_HEIGHT, 1);
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
