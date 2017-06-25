//******************************************************************************
//**************************** TILES *******************************************
//******************************************************************************

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
    board->block_count = 1;

    // Board_SetTile(board, 12, 0, {Metal, false, 0, 1, {1, 0, 0, 0}});
    // Board_SetTile(board, 12, 1, {Metal, false, 0, 1, {0, 0, 1, 0}});
    // Board_SetTile(board, 12, 2, {Metal, false, 0, 2, {1, 1, 0, 0}});
    // Board_SetTile(board, 13, 2, {Metal, false, 0, 2, {0, 0, 0, 1}});
    // Board_SetTile(board, 12, 3, {Metal, false, 0, 2, {1, 0, 1, 0}});
    // Board_SetTile(board, 12, 4, {Metal, false, 0, 2, {0, 1, 1, 0}});
    // Board_SetTile(board, 13, 4, {Metal, false, 0, 2, {0, 0, 0, 1}});
    // Board_SetTile(board, 13, 1, {Metal, false, 0, 3, {0, 1, 0, 0}});
    // Board_SetTile(board, 14, 1, {Metal, false, 0, 3, {1, 0, 0, 1}});
    // Board_SetTile(board, 14, 2, {Metal, false, 0, 3, {1, 0, 1, 0}});
    // Board_SetTile(board, 14, 3, {Metal, false, 0, 3, {0, 0, 1, 1}});
    // Board_SetTile(board, 13, 3, {Metal, false, 0, 3, {0, 1, 0, 0}});
    //
    //
    // Board_SetTile(board, 17, 0, {Metal, false, 0, 4, {0, 0, 0, 0}});
    // Board_SetTile(board, 15, 2, {Metal, false, 0, 5, {1, 1, 0, 0}});
    // Board_SetTile(board, 16, 2, {Metal, false, 0, 5, {0, 0, 0, 1}});
    // Board_SetTile(board, 15, 3, {Metal, false, 0, 5, {1, 0, 1, 0}});
    // Board_SetTile(board, 15, 4, {Metal, false, 0, 5, {0, 1, 1, 0}});
    // Board_SetTile(board, 16, 4, {Metal, false, 0, 5, {0, 0, 0, 1}});
    // Board_SetTile(board, 16, 1, {Metal, false, 0, 6, {0, 1, 0, 0}});
    // Board_SetTile(board, 17, 1, {Metal, false, 0, 6, {1, 0, 0, 1}});
    // Board_SetTile(board, 17, 2, {Metal, false, 0, 6, {1, 0, 1, 0}});
    // Board_SetTile(board, 17, 3, {Metal, false, 0, 6, {0, 0, 1, 1}});
    // Board_SetTile(board, 16, 3, {Metal, false, 0, 6, {0, 1, 0, 0}});
    //
    // Board_SetTile(board, 17, 4, {Metal, false, 0, 7, {0, 1, 0, 0}});
    // Board_SetTile(board, 18, 4, {Metal, false, 0, 7, {0, 1, 0, 1}});
    // Board_SetTile(board, 19, 4, {Metal, false, 0, 7, {0, 1, 0, 1}});
    // Board_SetTile(board, 20, 4, {Metal, false, 0, 7, {0, 0, 0, 1}});
    // Board_SetTile(board, 18, 5, {Metal, false, 0, 8, {0, 1, 0, 0}});
    // Board_SetTile(board, 19, 5, {Metal, false, 0, 8, {0, 1, 0, 1}});
    // Board_SetTile(board, 20, 5, {Metal, false, 0, 8, {0, 0, 0, 1}});
    // Board_SetTile(board, 18, 3, {Metal, false, 0, 9, {0, 1, 0, 0}});
    // Board_SetTile(board, 19, 3, {Metal, false, 0, 9, {0, 1, 0, 1}});
    // Board_SetTile(board, 20, 3, {Metal, false, 0, 9, {0, 0, 0, 1}});
    //
    // Board_SetTile(board, 19, 2, {Metal, false, 0, 10, {0, 0, 0, 0}});
    // Board_SetTile(board, 19, 2, {Metal, false, 0, 10, {0, 0, 0, 0}});
    //
    // Board_SetTile(board, 12, 7, {Metal, false, 0, 11, {0, 0, 0, 1}});
    // Board_SetTile(board, 11, 7, {Metal, false, 0, 11, {0, 1, 0, 0}});
    // Board_SetTile(board, 10, 7, {Metal, false, 0, 12, {1, 0, 0, 0}});
    // Board_SetTile(board, 10, 8, {Metal, false, 0, 12, {0, 0, 1, 0}});
    // Board_SetTile(board,  9, 9, {Metal, false, 0, 13, {0, 1, 0, 0}});
    // Board_SetTile(board, 10, 9, {Metal, false, 0, 13, {0, 1, 0, 1}});
    // Board_SetTile(board, 11, 9, {Metal, false, 0, 13, {0, 0, 0, 1}});
    //
    // board->block_count = 14;
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
