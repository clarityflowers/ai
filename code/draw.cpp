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

void DrawTile(PIXEL_BACKBUFFER* buffer, int x, int y, TILE tile, bool32 connections[4], bool32 shadow = false)
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
}

void DrawBlock(PIXEL_BACKBUFFER* buffer, GAME_BLOCK block, bool32 shadow = false)
{
    TILE_COORD coords[4];
    GetTileCoordsForBlock(coords, block);
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
        DrawTile(buffer, coords[i].x, coords[i].y, block.tiles[i], connections, shadow);
    }
}

void DrawBlockLanding(PIXEL_BACKBUFFER* buffer, BOARD *board)
{
    GAME_BLOCK landing_block = board->block;
    int landing = GetBlockLanding(board);
    if (landing != board->block.y)
    {
        landing_block.y = landing;
        DrawBlock(buffer, landing_block, true);
    }
}
