#include "alpha_cube.h"

#include <windows.h>
#include <stdio.h>
#include <SDL.h>
#include <math.h>


#define GAME_COLOR_DEPTH 2
#define GAME_PITCH (GAME_WIDTH * GAME_COLOR_DEPTH / 8)

#define BOARD_X 88
#define BOARD_Y 40

#include "vector.cpp"
#include "sound.cpp"
#include "random.h"
#include "draw.cpp"

#define SPRITEMAP_SIZE 4096

bool32 DEBUGLoadSpritemap(PixelBuffer* buffer, THREAD_CONTEXT* thread, debug_platform_read_entire_file *read, char* filename)
{
    DEBUG_READ_FILE_RESULT read_result = read(thread, filename);
    if(read_result.contents_size != 0)
    {
        uint8* pixel_quad = (uint8*)read_result.contents;
        uint8* pixel = buffer->pixels;
        for (uint32 i=0; (i < (128 * 128) / 4) && (i < read_result.contents_size); i++)
        {
            uint8 quad_value = *pixel_quad;
            for (int j=0; j < 4; j++)
            {
                uint8 color = (quad_value >> (3 - j) * 2) & 3;
                *pixel++ = color; 
            }
            pixel_quad++;
        }
        buffer->w = 128;
        buffer->h = 128;
        buffer->pitch = 128;
        return 1;
    }
    return 0;
}

bool32 DEBUGSaveSpritemap(PixelBuffer* spritemap, THREAD_CONTEXT* thread, debug_platform_read_entire_file *read, debug_platform_write_entire_file* write, char* filename)
{
    DEBUG_READ_FILE_RESULT read_result = read(thread, filename);
    if (read_result.contents_size != 0)
    {
        uint8* memory = (uint8*)read_result.contents;
        uint8* quad = memory;
        uint8* pixel = spritemap->pixels;
        for (uint32 i=0; (i < (128 * 128) / 4) && (i < read_result.contents_size); i++)
        {
            uint8 quad_value = 0x00;
            for (int j=0; j < 4; j++)
            {
                uint8 color = *pixel++;
                quad_value |= (color & 3) << ((3 - j) * 2);
            }
            *quad++ = quad_value;
        }

        bool32 result = write(thread, filename, read_result.contents_size, memory);
        return result;
    }

    return 0;

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
    if (print)
    {
        char FPSBuffer[256];
        snprintf(FPSBuffer, sizeof(FPSBuffer), "%.02f ms elapsed\n", milliseconds);
        OutputDebugStringA(FPSBuffer);
    }

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
            palette[j++] = {0xb0, 0xc1, 0x75, 0xFF};
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
    }
}


bool32 InRect(Coord pos, Rect rect)
{
    bool32 result = (pos.x > rect.pos.x) && (pos.x <= rect.pos.x + rect.w) && (pos.y > rect.pos.y) && (pos.y <= rect.pos.y + rect.h);
    return result;
}

bool32 InTileRect(Coord pos, TileRect rect)
{
    bool32 result = InRect(pos, ToRect(rect));
    return result;
}

bool32 InTile(Coord pos, TileCoord coord)
{
    bool32 result = InRect(pos, ToRect({coord.x, coord.y, 1, 1}));
    return result;
}

bool32 DebugButton(PixelBuffer* buffer, GAME_BUTTON_STATE click, Coord mouse_pos, TileRect s_rect)
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

void DrawText(PixelBuffer* buffer, PixelBuffer* spritemap, char* string, TileRect rect, uint8 first_sprite, int cursor_sprite)
{
    char c;
    int i = 0;
    c = string[i++];
    TileCoord pos = {0, 0};
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

            DrawTile(buffer, spritemap, rect.pos + pos, sprite);
        }
        if (c == '\n')
        {
            pos.x = rect.w;
        }
        else
        {
            pos.x++;
        }
        if (pos.x >= rect.w)
        {
            pos.x -= rect.w;
            pos.y--;
        }
        if (pos.y <= -rect.h)
        {
            break;
        }
        c = string[i++];
    }
    if (cursor_sprite >= 0 && cursor_sprite < 256)
    {
        DrawTile(buffer, spritemap, rect.pos + pos, (uint8)cursor_sprite);
    }
}

void DrawText(PixelBuffer* buffer, PixelBuffer* spritemap, char* string, TileRect rect, uint8 first_sprite)
{
    DrawText(buffer, spritemap, string, rect, first_sprite, -1);
}

void DrawDebugBorder(PixelBuffer* buffer, PixelBuffer* spritemap, TileRect rect, uint8 first_sprite)
{
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {rect.pos.x, i + rect.pos.y + 1};
        DrawTile(buffer, spritemap, where, first_sprite);
    }
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {i + rect.pos.x + 1, rect.pos.y + rect.h - 1};
        DrawTile(buffer, spritemap, where, first_sprite + 1);
    }
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {rect.pos.x + rect.w - 1, i + rect.pos.y + 1};
        DrawTile(buffer, spritemap, where, first_sprite + 2);
    }
    for (int i=0; i < (rect.w - 2); i++)
    {
        TileCoord where = {i + rect.pos.x + 1, rect.pos.y};
        DrawTile(buffer, spritemap, where, first_sprite + 3);
    }
    DrawTile(buffer, spritemap, {rect.pos.x, rect.pos.y + rect.h - 1}, first_sprite + 4);
    DrawTile(buffer, spritemap, {rect.pos.x + rect.w - 1, rect.pos.y + rect.h - 1}, first_sprite + 5);
    DrawTile(buffer, spritemap, {rect.pos.x + rect.w - 1, rect.pos.y}, first_sprite + 6);
    DrawTile(buffer, spritemap, {rect.pos.x, rect.pos.y}, first_sprite + 7);
}

uint8 LevelMap_GetBlock(LevelMap *map, BlockCoord pos)
{
    uint8 result = map->blocks[pos.y * LEVELMAP_WIDTH + pos.x];
    return result;
}

void LevelMap_SetBlock(LevelMap *map, PixelBuffer *spritemap, BlockCoord pos, uint8 block)
{
    map->blocks[pos.y * LEVELMAP_WIDTH + pos.x] = block;
    PixelBuffer buffer = {};
    buffer.pixels = map->pixels;
    buffer.w = LEVELMAP_WIDTH * BLOCK_SIZE;
    buffer.h = LEVELMAP_HEIGHT * BLOCK_SIZE;
    buffer.pitch = buffer.w;
    TileCoord tile_pos = ToTileCoord(pos);
    if (block == 0)
    {
        TileCoord sprite = {0,0};
        DrawTile(&buffer, spritemap, tile_pos,                   sprite);
        DrawTile(&buffer, spritemap, tile_pos + TileCoord{1, 0}, sprite);
        DrawTile(&buffer, spritemap, tile_pos + TileCoord{0, 1}, sprite);
        DrawTile(&buffer, spritemap, tile_pos + TileCoord{1, 1}, sprite);
    }
    else
    {
        DrawTile(&buffer, spritemap, tile_pos, {{2, 0}, 2, 2});
    }
}


GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    int selected_palette = 0;

    uint64 timer = StartProfiler();

    Assert(sizeof(GAME_STATE) <= memory->permanent_storage_size);
    GAME_STATE *state = (GAME_STATE*) memory->permanent_storage;


    bool32 reset_tile_map = 0;
    bool32 first = 0;

    // MARK: Initialize
    if (!memory->is_initialized)
    {
        first = 1;
        PixelBuffer *buffer = &(state->buffer);
        
        state->spritemap.pixels = (uint8*)&state->spritemap_pixels;
        state->debug_spritemap.pixels = (uint8*)&state->debug_spritemap_pixels;
        DEBUGLoadSpritemap(&state->spritemap, thread, memory->DEBUGPlatformReadEntireFile, "spritemap.sm");
        DEBUGLoadSpritemap(&state->debug_spritemap, thread, memory->DEBUGPlatformReadEntireFile, "debug_spritemap.sm");
        
        buffer->pixels = (uint8*) &state->buffer_pixels;
        buffer->w = GAME_WIDTH;
        buffer->h = GAME_HEIGHT;
        buffer->pitch = buffer->w;
        memory->is_initialized = true;
        state->clock.bpm = 120.0f;
        state->clock.meter = 4;
        state->mode = 0;

        memory_index used_memory = sizeof(GAME_STATE);
        InitializeArena(&state->arena, Kilobytes(32), (uint8 *) memory->permanent_storage + used_memory);
        used_memory += Kilobytes(32);

        {
            state->player_position.y = 32;
        }

        reset_tile_map = 1;

        state->debug_frequency = 46.8509750f;
        state->triangle_channel.num_harmonics = 3;

        state->copying = 0;
        state->muted = 1;
    }

    int key = -1;
    if (input->key_buffer_length > 0)
    {
        key = input->key_buffer[input->key_buffer_position];
        input->key_buffer_position++;
        input->key_buffer_position %= 256;
        input->key_buffer_length--;
    }

    if (key == '`')
    {
        state->mode = (state->mode == 1) ? 0 : 1;
    }

    if (key == 'm')
    {
        state->muted = (state->muted) ? 0 : 1; 
    }


    if (key == '-') state->debug_frequency -= 0.1f;
    if (key == '=') state->debug_frequency += 0.1f;
    if (key == '[') state->debug_frequency -= 0.01f;
    if (key == ']') state->debug_frequency += 0.01f;

    if (key == ';') state->triangle_channel.num_harmonics--;
    if (key == '\'') state->triangle_channel.num_harmonics++;
    
    if (state->mode == 0) // MARK: Play 
    {
        PixelBuffer* buffer = &state->buffer;
        PixelBuffer* spritemap = &state->spritemap;
        LevelMap* level_map = &state->level_map;


        if (key == 'r')
        {
            reset_tile_map = 1;
        }
    
        if (reset_tile_map)
        {
            for (int y=0; y < LEVELMAP_HEIGHT; y++)
            {
                for (int x=0; x < LEVELMAP_WIDTH; x++)
                {
                    LevelMap_SetBlock(level_map, spritemap, {x, y}, 0);
                }
            }
            for (int y=0; y < 2; y++)
            {
                for (int x=0; x < 16; x++)
                {
                    LevelMap_SetBlock(level_map, spritemap, {x, y}, 1);
                }
            }
            LevelMap_SetBlock(level_map, spritemap, {7, 3}, 1);
            LevelMap_SetBlock(level_map, spritemap, {8, 3}, 1);
            LevelMap_SetBlock(level_map, spritemap, {8, 4}, 1);
            LevelMap_SetBlock(level_map, spritemap, {9, 4}, 1);
            LevelMap_SetBlock(level_map, spritemap, {10, 4}, 1);
            state->camera = {0, 0};

            state->player_position = {0,2};
            state->player_velocity = {0.0};
            state->player_dash_frames = 0;
        }

        // MARK: Player physics
        {
            GAME_CONTROLLER_INPUT* controller = &input->controllers[0];
            V2 movement_force = {};
            if (controller->right.is_down)
            {
                if (!state->player_dash_frames)
                {
                    movement_force.x += MOVEMENT_ENERGY;
                    state->player_facing_right = 1;
                }
            }
            else if (controller->left.is_down)
            {
                if (!state->player_dash_frames)
                {
                    movement_force.x -= MOVEMENT_ENERGY;
                    state->player_facing_right = 0;
                }
            }
            if (controller->a.was_pressed)
            {
                if (state->player_on_ground)
                {
                    state->player_velocity.y = JUMP_SPEED;
                    state->player_on_ground = 0;
                }
            }
            if (controller->b.was_pressed)
            {
                if (!state->player_dash_frames)
                {
                    state->player_dash_frames = DASH_START;
                    state->player_velocity = {DASH_SPEED, 0};
                    if (!state->player_facing_right)
                    {
                        state->player_velocity.x *= -1;
                    }
                }
            }

            V2 gravity_force = {0, -GRAVITY_ENERGY};
            if (controller->a.is_down && state->player_velocity.y > 0)
            {
                gravity_force.y *= JUMPING_GRAVITY_MOD;
            }
            if (state->player_dash_frames)
            {
                gravity_force.y = 0.0f;
            }

            V2 friction_force = {};
            if (state->player_velocity.x > 0.1f)
            {
                friction_force.x = -FRICTION_ENERGY;
            }
            else if (state->player_velocity.x < -0.1f)
            {
                friction_force.x = FRICTION_ENERGY;
            }
            else
            {
                state->player_velocity.x = 0.0f;
            }

            
            float vel = state->player_velocity.x;
            V2 drag_force = {};
            float drag_coefficient = DRAG_COEF;
            if (state->player_dash_frames > DASH_HANG)
            {
                drag_coefficient *= DASH_DRAG_COEF_MOD;
            }
            else if (state->player_dash_frames > 0)
            {
                drag_coefficient *= DASH_HANGING_DRAG_COEF_MOD;
            }
            drag_force.x = -vel * (float)fabs(vel) * drag_coefficient;
            
            V2 force = movement_force + friction_force + drag_force + gravity_force;
            float mass = 1.0f;
            V2 acceleration = force / mass;
            
            state->player_velocity += acceleration;

            V2 movement = state->player_velocity;
            float distance = (float)max(1, ceil(max(fabs(movement.x), fabs(movement.y)))) * 8;
            V2 interval = movement / distance;
            Rect collider = {{4, 0}, 6, 14}; 
            int right = collider.pos.x + collider.w - 1;
            int top = collider.pos.y + collider.h - 1;
            Coord checks[4] = {collider.pos, {right, collider.pos.y}, {right, top}, {collider.pos.x, top}};
            for (int i=0; i < distance; i++) 
            {
                {
                    V2 new_position = state->player_position + V2{interval.x, 0};
                    bool32 move = 1;
                    Coord new_coord = ToCoord(new_position);
                    for (int p=0; p < 4; p++)
                    {
                        BlockCoord block_coord = ToBlockCoord(new_coord + checks[p]);
                        int block = LevelMap_GetBlock(level_map, block_coord);
        
                        if (block >= 1 || block_coord.x >= 16 || block_coord.x < 0)
                        {
                            move = 0;
                            interval.x = 0.0f;
                            state->player_velocity.x = 0.0f;
                        }
                    }
                    if (move)
                    {
                        state->player_position = new_position;
                    }
                }
                {
                    V2 new_position = state->player_position + V2{0, interval.y};
                    bool32 move = 1;
                    Coord new_coord = ToCoord(new_position);
                    for (int p=0; p < 4; p++)
                    {

                        BlockCoord block_coord = ToBlockCoord(new_coord + checks[p]);
                        int block = LevelMap_GetBlock(level_map, block_coord);
        
                        if (block >= 1)
                        {
                            if (p < 2) {
                                state->player_on_ground = 1;
                                state->coyote_time = 0;
                            }
                            move = 0;
                            interval.y = 0.0f;
                            state->player_velocity.y = 0.0f;
                        }
                    }
                    if (move)
                    {
                        if (interval.y < 0 && state->player_on_ground)
                        {
                            if (state->coyote_time < 32)
                            {
                                state->coyote_time++;
                            }
                            else
                            {
                                state->player_on_ground = 0;
                            }
                        }
                        state->player_position = new_position;
                    }
                }
            }

            if (state->player_dash_frames) state->player_dash_frames--;
        }


        // MARK: Draw
        {
            {
                uint8* set_row = buffer->pixels;
                uint8* get_row = ((uint8*)level_map->pixels); // TODO: add camera pos
                for (int y=0; y < GAME_HEIGHT; y++)
                {
                    memcpy(set_row, get_row, CAMERA_WIDTH * BLOCK_SIZE);
                    set_row += buffer->pitch;
                    get_row += CAMERA_WIDTH * BLOCK_SIZE;

                }
            }

            TileCoord player_sprite;
            if (state->player_facing_right)
            {
                if (state->player_dash_frames) player_sprite = {4, 2};
                else player_sprite = {0, 2};
            }
            else
            {
                if (state->player_dash_frames) player_sprite = {6, 2};
                else player_sprite = {2, 2};
            }
            DrawSprite(buffer, spritemap, ToCoord(state->player_position), TileRect{player_sprite, 2, 2}, 2);

        }
    }
    else if (state->mode == 1) // MARK: Edit
    {
        PixelBuffer* buffer = &state->buffer;
        bool32 save = 0;
        PixelBuffer* current_spritemap = &state->spritemap;

        if (key >= '1' && key <= '8')
        {
            int selection = key - '0' - 1;
            if (selection < 8)
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
            save = 1;
        }

        GAME_BUTTON_STATE click = input->primary_click;
        Coord pos;
        {
            float x_scale = ((float)state->buffer.w) / render_buffer->w;
            float y_scale = ((float)state->buffer.h) / render_buffer->h;
            pos.x = (int)round(input->mouse_x * x_scale); 
            pos.y = (int)round(input->mouse_y * y_scale); 
        }



        if (state->edit_mode == 0) // MARK: Selection mode
        {
            if (key == 'c')
            {
                state->copying = state->copying ? 0 : 1;
            }

            DrawClear(&state->buffer, 3);
            DrawDebugBorder(&state->buffer, &state->debug_spritemap, {7, 4, 18, 18}, 16);
            for (uint8 row=0; row < 16; row++)
            {
                for (uint8 col=0; col < 16; col++)
                {
                    TileCoord tile = {col, row};
                    TileCoord where = {col + 8, row + 5};
                    DrawTile(&state->buffer, current_spritemap, where, tile);
                    if (InTile(pos, where))
                    {
                        if (click.was_pressed)
                        {
                            if (state->copying)
                            {
                                CopySprite(current_spritemap, tile, state->selected_sprites[state->current_sprite]);
                            }
                            else
                            {
                                state->selected_sprites[state->current_sprite] = {tile, 1, 1};
                                state->select_start = tile;
                                state->selecting_sprite = 1;
                            }
                        }
                        if (state->selecting_sprite)
                        {
                            TileCoord top_left;
                            top_left.x = min(state->select_start.x, col);
                            top_left.y = min(state->select_start.y, row);
                            int width = max(state->select_start.x, col) - top_left.x + 1;
                            int height = max(state->select_start.y, row) - top_left.y + 1;
                            state->selected_sprites[state->current_sprite] = {top_left, width, height};
                        }
                    }
                }
            }

            if (state->selecting_sprite && click.was_released)
            {
                state->selecting_sprite = 0;
            }
        }
        else // MARK: Drawing mode
        {
            TileRect current_sprite = state->selected_sprites[state->current_sprite];
            switch (key) 
            {
                case 'c':
                {
                    for (int x=0; x < current_sprite.w * 8; x++)
                    {
                        for (int y=0; y < current_sprite.h * 8; y++)
                        {
                            DrawPointInSprite(current_spritemap, {x, y}, current_sprite, state->cursor_color);
                        }
                    }
                } break;
                case 'f':
                {
                    int w = current_sprite.w * 8;
                    for (int y=0; y < current_sprite.h * 8; y++)
                    {
                        for (int x=0; x < w / 2; x++)
                        {
                            Coord left_point = {x, y};
                            Coord right_point = {w - 1 - x, y};
                            uint8 left = GetPointFromSprite(current_spritemap, left_point, current_sprite);
                            uint8 right = GetPointFromSprite(current_spritemap, right_point, current_sprite);
                            DrawPointInSprite(current_spritemap, left_point, current_sprite, right);
                            DrawPointInSprite(current_spritemap, right_point, current_sprite, left);
                        }
                    }
                } break;
                case 'q':
                {
                    state->cursor_color = 0;
                } break;
                case 'w':
                {
                    state->cursor_color = 1;
                } break;
                case 'e':
                {
                    state->cursor_color = 2;
                } break;
                case 'r':
                {
                    state->cursor_color = 3;
                } break;
                case 'x':
                {
                    state->swap_colors = state->swap_colors ? 0 : 1;
                } break;
                default: break;
            }
            DrawClear(buffer, 3);
            DrawDebugBorder(buffer, &state->debug_spritemap, {3, 2, 26, 26}, 16);
            
            for (uint8 color=0; color < 4; color++)
            {
                TileRect wide_rect = {32 - 3, 3 + (3 * 2 * color), 3, 3 * 2};
                TileRect narrow_rect = {32 - 3, 3 + (3 * 2 * color), 2, 3 * 2};
                DrawRectAligned(buffer, narrow_rect, color);
                if (InTileRect(pos, wide_rect) && click.was_released)
                {
                    if (state->swap_colors)
                    {
                        Rect rect = ToRect(current_sprite);
                        for (int x=0; x < rect.w; x++)
                        {
                            for (int y=0; y < rect.h; y++)
                            {
                                Coord coord = {x, y};
                                uint8 pixel = GetPointFromSprite(current_spritemap, coord, current_sprite);
                                if (pixel == color)
                                {
                                    DrawPointInSprite(current_spritemap, coord, current_sprite, state->cursor_color);
                                }
                                else if (pixel == state->cursor_color)
                                {
                                    DrawPointInSprite(current_spritemap, coord, current_sprite, color);
                                }
                            }
                        }
                        state->swap_colors = 0;
                    }
                    state->cursor_color = color;
                }
            }
            DrawRectAligned(buffer, {31, 3, 3, 3 * 2 * 4}, state->cursor_color);

            if (state->swap_colors)
            {
                DrawRectAligned(buffer, {{31, 29}, 1, 1}, 0);
            }

            // pixels
            {
                Rect to_rect = ToRect(TileRect{{4, 3}, 24, 24});
                Rect from_rect = ToRect(current_sprite);

                float scale_factor = (float)fmax(current_sprite.w / 24.0f, current_sprite.h / 24.0f);

                bool32 is_hovering = 0;
                Coord hovered_tile = {};

                for (int row=0; row < to_rect.w; row++)
                {
                    for (int col=0; col < to_rect.h; col++)
                    {
                        Coord get = {(int)(col * scale_factor), (int)(row * scale_factor)};
                        if (get.x < from_rect.w && get.y < from_rect.h)
                        {
                            uint8 color = GetPointFromSprite(current_spritemap, get, current_sprite);
                            Coord draw_point = {to_rect.pos.x + col, to_rect.pos.y + row};
                            DrawPoint(buffer, draw_point, color);

                            if (pos == draw_point)
                            {
                                is_hovering = 1;
                                hovered_tile = get;
                            }

                        }
                    }
                }

                if (is_hovering)
                {
                    if (click.is_down)
                    {
                        DrawPointInSprite(current_spritemap, hovered_tile, current_sprite, state->cursor_color);
                    }
                    else
                    {
                        Rect hover_rect = {};
                        hover_rect.pos.x = (int)((hovered_tile.x + 0.33f) / scale_factor + (4 * 8));
                        hover_rect.pos.y = (int)((hovered_tile.y + 0.33f) / scale_factor + (3 * 8));
                        hover_rect.w = (int)(0.4f / scale_factor);
                        hover_rect.h = (int)(0.4f / scale_factor);
                        DrawRect(buffer, hover_rect, state->cursor_color);
                    }
                }
            }   
        }


        for (int i=0; i < 8; i++)
        {
            TileRect sprite = state->selected_sprites[i];
            int y = 22 - i * 3;
            if (sprite.w > 0)
            {
                DrawScaledTile(buffer, current_spritemap, {{1, y}, 2, 2}, sprite);
            }
            if (sprite.w > 0 || state->current_sprite == i)
            {
                uint8 bubble;
                if (state->current_sprite == i)
                {
                    if (state->edit_mode == 0 && state->copying)
                    {
                        bubble = 4;
                    }
                    else
                    {
                        bubble = 3;
                    }
                }
                else
                {
                    bubble = 2;
                }
                DrawTile(buffer, &state->debug_spritemap, {2, y + 2}, bubble);
            }
        }


        {
            uint8 sprite = click.is_down ? 1 : 0;
            DrawSprite(buffer, &state->debug_spritemap, {pos.x, pos.y - 8}, sprite, 0);
            // uint8 color = click.is_down ? 2 : 3;
            // DrawRect(&state->buffer, pos.x, pos.y - 4, 4, 4, color);
        }
        if (save)
        {
            DEBUGSaveSpritemap(&state->spritemap, thread, memory->DEBUGPlatformReadEntireFile, memory->DEBUGPlatformWriteEntireFile, "spritemap.sm");
        }
        // DrawText(&state->buffer, &state->spritemap, "HERE IS A SENTENCE.\nAND HERE IS ANOTHER.", {0, 29, 32, 4}, 32);
    }

#if 0
    if (1)
    {
        PixelBuffer* buffer = &state->buffer;
        DrawClear(buffer, 0);
        DrawRect(buffer, {{0, 181}, 256, 60}, 3);
        DrawRect(buffer, {{0, 0}, 256, 60}, 3);
        for (int i=0; i < 256; i++)
        {
            float32 value = 0.0;
            for (int j=0; j < 4; j++)
            {
                value += state->audio_buffer[i * 4 + j];
            }
            value = value / 4.0f;
            value = (value + 1.0f) / 2.0f;
            int height = (int)(value * 120.0f) + 1;
            DrawRect(buffer, {{i, 60}, 1, height}, 2);
        }
    }
#endif
    
    // MARK: Copy onto backbuffer
    {
        PixelBuffer *buffer = &state->buffer;

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

        pixel_row = buffer->pixels + ((buffer->h - 1) * buffer->pitch);

        RGBA_COLOR palette[4];
        GetPalette(palette, selected_palette);

        RGBA_COLOR* render_pixel = (RGBA_COLOR*)render_buffer->pixels;

        for (int row = (GAME_HEIGHT - 1); row >= 0; row--)
        {
            for (int i=0; i < scale_factor; i++)
            {
                uint8* pixel = pixel_row;

                for (int col=0; col < GAME_WIDTH; col++)
                {
                    RGBA_COLOR color;

                    color = palette[*pixel];
                    for (int j=0; j < scale_factor; j++)
                    {
                        *render_pixel++ = color;
                    }
                    pixel++;
                }
            }
            pixel_row -= buffer->pitch;
        }
    }
    StopProfiler(timer, false);
    return 1;
}
