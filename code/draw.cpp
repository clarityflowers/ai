Rect ToRect(TileRect tile_rect)
{
    Rect result = {tile_rect.x * 8, tile_rect.y * 8, tile_rect.w * 8, tile_rect.h * 8};
    return result;
}

uint8 GetPoint(PIXEL_BACKBUFFER* buffer, Coord pos)
{
    uint8 pixel_quad = *((uint8*)buffer->pixels + (pos.x / 4) + (pos.y * buffer->pitch));
    int position = (pos.x % 4) * 2;
    uint8 result = (pixel_quad >> position) & 3;
    Assert(result >= 0 && result < 4);
    return result;
}

TileCoord SpriteToTileCoord(uint8 sprite)
{
    TileCoord result = {sprite % 16, sprite / 16};
    return result;
}

Coord SpriteToCoord(uint8 sprite)
{
    Coord result = ToCoord(SpriteToTileCoord(sprite));
    return result;
}

uint8 GetPointFromSprite(PIXEL_BACKBUFFER* buffer, Coord pos, uint8 sprite)
{
    Coord true_pos = SpriteToCoord(sprite) + pos; 
    return GetPoint(buffer, true_pos);
}

void DrawPoint(PIXEL_BACKBUFFER* buffer, int x, int y, uint8 color)
{
    uint8* pixel_quad = (uint8*) buffer->pixels + (x / 4) + (y * buffer->pitch);
    int position = (x % 4) * 2;
    uint8 mask = 3 << position;
    *pixel_quad = (*pixel_quad & ~mask) | (color << position);
}

void DrawPoint(PIXEL_BACKBUFFER* buffer, Coord pos, uint8 color)
{
    DrawPoint(buffer, pos.x, pos.y, color);
}

void DrawPointInSprite(PIXEL_BACKBUFFER* buffer, Coord pos, uint8 sprite, uint8 color)
{

    DrawPoint(buffer, SpriteToCoord({sprite}) + pos, color);
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

void DrawRect(PIXEL_BACKBUFFER* buffer, Rect rect, uint8 color)
{
    DrawRect(buffer, rect.x, rect.y, rect.w, rect.h, color);
}

void DrawRectAligned(PIXEL_BACKBUFFER* buffer, TileRect rect, uint8 color)
{
    DrawRect(buffer, ToRect(rect), color);
}

void DrawSprite(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, Coord buffer_pos, uint8 sprite, int transparent)
{
    int start_x = (sprite % 16) * 8;
    Coord get_pos = {start_x, (sprite / 16) * 8};
    Coord draw_pos = buffer_pos;
    for (int row=0; row < 8; row++)
    {
        if (draw_pos.y >= 0 && draw_pos.y < buffer->w)
        {
            get_pos.x = start_x;
            draw_pos.x = buffer_pos.x;
            for (int col=0; col < 8; col++)
            {
                if (draw_pos.x >= 0 && draw_pos.x < buffer->w)
                {
                    uint8 color = GetPoint(spritemap, get_pos);
                    if (transparent != color)
                    {
                        DrawPoint(buffer, draw_pos, color);
                    }
                }
                get_pos.x++;
                draw_pos.x++;
            }
        }
        get_pos.y++;
        draw_pos.y++;
    }
}

void DrawSprite(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, Coord buffer_pos, uint8 sprite)
{
    DrawSprite(buffer, spritemap, buffer_pos, sprite, -1);
}

void DrawSpriteAligned(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, TileCoord buffer_pos, uint8 sprite, int transparent)
{
    DrawSprite(buffer, spritemap, ToCoord(buffer_pos), sprite, transparent);
}

void DrawSpriteAligned(PIXEL_BACKBUFFER* buffer, PIXEL_BACKBUFFER* spritemap, TileCoord buffer_pos, uint8 sprite)
{
    DrawSpriteAligned(buffer, spritemap, buffer_pos, sprite, -1);
}

void DrawClear(PIXEL_BACKBUFFER* buffer, uint8 color)
{
    DrawRect(buffer, 0, 0, GAME_WIDTH, GAME_HEIGHT, color);
}

void DrawTile(PIXEL_BACKBUFFER* buffer, TILE_COORD coord, TILE tile, bool32 connections[4], bool32 shadow = false)
{
    int screen_x, screen_y;
    if (coord.x >= 0 && coord.x < BOARD_WIDTH && coord.y >= 0 && coord.y < BOARD_HEIGHT)
    {
        screen_x = coord.x * 8;
        screen_y = BOARD_Y + coord.y * 8;
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

        if (!connections[0])
        {
            DrawRect(buffer, screen_x + 1, screen_y + 7, 6, 1, 0);
            if (connections[1])
            {
                DrawPoint(buffer, screen_x + 7, screen_y + 7, 0);
            }
        }
        if (!connections[1])
        {
            DrawRect(buffer, screen_x + 7, screen_y + 1, 1, 6, 3);
        }
        if (!connections[2])
        {
            DrawRect(buffer, screen_x + 1, screen_y, 6, 1, 3);
        }
        if (connections[3])
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
        DrawRect(buffer, screen_x + 2, screen_y + 2, 4, 4, 3);
        if (connections[0])
        {
            DrawRect(buffer, screen_x + 2, screen_y + 6, 4, 2, 3);
        }
        if (connections[1])
        {
            DrawRect(buffer, screen_x + 6, screen_y + 2, 2, 4, 3);
        }
        if (connections[2])
        {
            DrawRect(buffer, screen_x + 2, screen_y, 4, 2, 3);
        }
        if (connections[3])
        {
            DrawRect(buffer, screen_x, screen_y + 2, 2, 4, 3);
        }
        if (tile.on_fire) {
            DrawRect(buffer, screen_x + 2, screen_y + 2, 4, 4, 1);
        }
    }
    else if (tile.kind == Fire)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 3);
        DrawRect(buffer, screen_x + 1, screen_y + 1, 6, 6, 1);
    }
    else if (tile.kind == Seed)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 2);
    }
    else if (tile.kind == Vine)
    {
        DrawRect(buffer, screen_x, screen_y, 8, 8, 3);
    }
}

void DrawBlock(PIXEL_BACKBUFFER* buffer, GAME_BLOCK block, bool32 shadow = false)
{
    TILE_COORD coords[4];
    GetTileTILE_COORDsForBlock(coords, block);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        bool32 connections[4] = {};
        for (int j=0; j < block.def->num_tiles; j++)
        {
            if (i != j)
            {
                if (coords[i].x == coords[j].x && coords[i].y + 1 == coords[j].y)
                {
                    connections[0] = true;
                }
                if (coords[i].x + 1 == coords[j].x && coords[i].y == coords[j].y)
                {
                    connections[1] = true;
                }
                if (coords[i].x == coords[j].x && coords[i].y - 1 == coords[j].y)
                {
                    connections[2] = true;
                }
                if (coords[i].x -1 == coords[j].x && coords[i].y == coords[j].y)
                {
                    connections[3] = true;
                }
            }
        }
        DrawTile(buffer, coords[i], block.tiles[i], connections, shadow);
    }
}



void DrawBlockLanding(PIXEL_BACKBUFFER* buffer, BOARD *board)
{
    GAME_BLOCK landing_block = board->block;
    int landing = GetBlockLanding(board);
    if (landing != board->block.pos.y)
    {
        landing_block.pos.y = landing;
        DrawBlock(buffer, landing_block, true);
    }
}
