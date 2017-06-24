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
}
