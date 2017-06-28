#if !defined(ALPHA_CUBE_H)

union TILE_COORD
{
    struct
    {
        int x, y;
    };
    int e[2];
};

TILE_COORD operator+(TILE_COORD a, TILE_COORD b)
{
    TILE_COORD result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

TILE_COORD operator+=(TILE_COORD &a, TILE_COORD b)
{
    a = a + b;

    return a;
}

#define CUBE_MATH_H
#endif
