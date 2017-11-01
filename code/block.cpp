void GetTileTILE_COORDsForBlock(TILE_COORD coords[4], GAME_BLOCK block)
{
    TILE_OFFSET offsets[4];
    RotateTileOffsets(offsets, block.def->offsets, block.def->num_tiles, block.rotate);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        coords[i] = offsets[i] + block.pos;
    }
}

int GetBlockLanding(BOARD *board)
{
    TILE_COORD coords[4];
    GAME_BLOCK block = board->block;
    GetTileTILE_COORDsForBlock(coords, block);
    int step_down;
    for (step_down = 0; step_down < block.pos.y; step_down++)
    {
        for (int i=0; i < block.def->num_tiles; i++)
        {
            TILE_COORD coord = coords[i];
            coord.y -= step_down + 1;
            if (Board_TileExists(board, coord))
            {
                goto END;
            }
        }
    }
    END:
    int result = block.pos.y - step_down;
    return result;
}


bool32 BlockIsValid(BOARD *board, GAME_BLOCK block)
{
    bool32 is_valid = true;
    TILE_COORD coords[4];
    GetTileTILE_COORDsForBlock(coords, block);
    for (int i=0; i < block.def->num_tiles; i++)
    {
        if (Board_TileExists(board, coords[i]))
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
        block.pos.x++;
        distance--;
        if (!BlockIsValid(board, block)) return false;
    }
    while (distance < 0)
    {
        block.pos.x--;
        distance++;
        if (!BlockIsValid(board, block)) return false;
    }
    return true;
}

bool32 BlockCanMoveVertically(BOARD *board, GAME_BLOCK block, int distance)
{
    while (distance > 0)
    {
        block.pos.y++;
        distance --;
        if (!BlockIsValid(board, block)) return false;
    }
    return true;
}

bool32 BlockTryMoveHorizontally(BOARD *board, GAME_BLOCK *block, int distance)
{
    if (!BlockCanMoveHorizontally(board, *block, distance)) return false;
    block->pos.x += distance;
    return true;
}

bool32 BlockTryMoveVertically(BOARD *board, GAME_BLOCK *block, int distance)
{
    if (!BlockCanMoveVertically(board, *block, distance)) return false;
    block->pos.y += distance;
    return true;
}

bool32 BlockCanRotate(BOARD *board, GAME_BLOCK block, int dr)
{
    RotateBlock(&block, dr);
    return BlockIsValid(board, block);
}

bool32 BlockCanMove(BOARD *board, GAME_BLOCK block, TILE_COORD dxy, int dr)
{
    return BlockTryMoveVertically(board, &block, dxy.y) &&
           BlockTryMoveHorizontally(board, &block, dxy.x) &&
           BlockCanRotate(board, block, dr);
}

bool32 BlockTryRotate(BOARD *board, int dr)
{
    if (!BlockCanRotate(board, board->block, dr)) return false;
    RotateBlock(&board->block, dr);
    return true;
}

bool32 BlockTryMove(BOARD *board, TILE_COORD dxy, int dr)
{
    if (BlockCanMove(board, board->block, dxy, dr))
    {
        board->block.pos += dxy;
        RotateBlock(&board->block, dr);
        return true;
    }
    return false;
}