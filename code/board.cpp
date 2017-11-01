//******************************************************************************
//**************************** TILES *******************************************
//******************************************************************************

TILE Board_GetTile(BOARD *board, TILE_COORD coord)
{
    TILE result = {};
    if (coord.x >= 0 && coord.x < board->width && coord.y >= 0 && coord.y < board->height)
    {
        result = board->tiles[(coord.y * board->width) + coord.x];
    }
    return result;
}

bool32 Board_TileExists(BOARD *board, TILE_COORD coord)
{
    bool32 result;
    if (coord.x >= 0 && coord.x < board->width && coord.y >= 0 && coord.y < board->height)
    {
        TILE tile = board->tiles[(coord.y * board->width) + coord.x];
        result = (tile.kind > 0);
    }
    else
    {
        result = 1;
    }
    return result;
}

void Board_SetTile(BOARD *board, TILE_COORD coord, TILE value)
{
    board->tiles[(coord.y * board->width) + coord.x] = value;
}

void Board_ClearTile(BOARD *board, TILE_COORD coord)
{
    TILE tile = {};
    Board_SetTile(board, coord, tile);
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
