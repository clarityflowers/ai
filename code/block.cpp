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

int GetBlockLanding(BOARD *board)
{
    TILE_COORD coords[4];
    GAME_BLOCK block = board->block;
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

bool32 BlockTryRotate(BOARD *board, int dr)
{
    if (!BlockCanRotate(board, board->block, dr)) return false;
    RotateBlockTiles(&board->block, dr);
    return true;
}

bool32 BlockTryMove(BOARD *board, int dx, int dy, int dr)
{
    if (BlockCanMove(board, board->block, dx, dy, dr))
    {
        board->block.x += dx;
        board->block.y += dy;
        RotateBlockTiles(&board->block, dr);
        return true;
    }
    return false;
}
