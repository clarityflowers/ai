#include "platform.h"
#include "vector.h"


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

uint8* GetPixel(PixelBuffer* buffer, Coord pos)
{
    uint8* result = buffer->pixels + (pos.y * buffer->pitch + pos.x);
    return result;
}

uint8 GetPoint(PixelBuffer* buffer, Coord pos)
{
    Assert(pos.x >= 0 && pos.y >= 0);
    uint8 result = *GetPixel(buffer, pos);
    Assert(result >= 0 && result < 4);
    return result;
}

uint8 GetPointFromSprite(PixelBuffer* buffer, Coord pos, uint8 sprite)
{
    Assert(pos.x < 8 && pos.y < 8);
    Coord true_pos = SpriteToCoord(sprite) + pos; 
    return GetPoint(buffer, true_pos);
}

uint8 GetPointFromSprite(PixelBuffer* buffer, Coord pos, TileRect sprite)
{
    Rect rect = ToRect(sprite);
    Assert(pos.x < rect.w && pos.y < rect.h);
    uint8 result = GetPoint(buffer, rect.pos + pos);
    return result;
}

void DrawPoint(PixelBuffer* buffer, Coord pos, uint8 color)
{
    Assert(pos.x >= 0 && pos.y >= 0);
    uint8* pixel = GetPixel(buffer, pos);
    *pixel = color;
}

void DrawPointInSprite(PixelBuffer* buffer, Coord pos, uint8 sprite, uint8 color)
{
    Assert(pos.x < 8 && pos.y < 8);
    DrawPoint(buffer, SpriteToCoord(sprite) + pos, color);
}

void DrawPointInSprite(PixelBuffer* buffer, Coord pos, TileCoord sprite, uint8 color)
{
    Assert(pos.x < 8 && pos.y < 8);
    DrawPoint(buffer, ToCoord(sprite) + pos, color);
}

void DrawPointInSprite(PixelBuffer* buffer, Coord pos, TileRect sprite, uint8 color)
{
    Rect rect = ToRect(sprite);
    Assert(pos.x < rect.w && pos.y < rect.h);
    DrawPoint(buffer, rect.pos + pos, color);
}

Rect Trim(Rect rect, Rect canvas)
{
    Rect result = rect;
    if (result.pos.x < canvas.pos.x)
    {
        int clip = canvas.pos.x - result.pos.x;
        result.pos.x += clip;
        result.w -= clip;
    }
    if (result.pos.y < canvas.pos.y)
    {
        int clip = canvas.pos.y - result.pos.y;
        result.pos.y += clip;
        result.h -= clip;
    }
    int right = result.pos.x + result.w;
    int right_edge = canvas.pos.x + canvas.w;  
    if (right > right_edge)
    {
        int clip = right - (right_edge);
        result.w -= clip;
    }
    int left = result.pos.y + result.h;
    int left_edge = canvas.pos.y + canvas.h;  
    if (left > left_edge)
    {
        int clip = left - (left_edge);
        result.h -= clip;
    }

    return result;
}

Rect Trim(Rect rect, int w, int h)
{
    Rect result = Trim(rect, {0, 0, w, h});
    return result;
}

void DrawImage(PixelBuffer* to_buffer, PixelBuffer* from_buffer, Coord to_pos, Rect from_rect)
{
    Rect to_rect = Trim({to_pos, from_rect.w, from_rect.h}, to_buffer->w, to_buffer->h);
    Coord from_pos = from_rect.pos + (to_rect.pos - to_pos); 

    uint8* from_row = GetPixel(from_buffer, from_pos);
    uint8* to_row = GetPixel(to_buffer, to_rect.pos);

    for (int y=0; y < to_rect.h; y++)
    {
        memcpy(to_row, from_row, to_rect.w);
        from_row += from_buffer->pitch;
        to_row += to_buffer->pitch;
    }
}

void DrawSprite(PixelBuffer* buffer, PixelBuffer* spritemap, Coord to_pos, TileRect sprite, uint8 transparent)
{
    Rect sprite_rect = ToRect(sprite);
    Rect to_rect = Trim({to_pos, sprite_rect.w, sprite_rect.h}, buffer->w, buffer->h);
    Coord from_pos = sprite_rect.pos + (to_rect.pos - to_pos);

    uint8* from_row = GetPixel(spritemap, from_pos);
    uint8* to_row = GetPixel(buffer, to_rect.pos);

    for (int y=0; y < to_rect.h; y++)
    {
        uint8* from_pixel = from_row;
        uint8* to_pixel = to_row;
        for (int x=0; x < to_rect.w; x++)
        {
            uint8 color = *from_pixel;
            if (color != transparent)
            {
                *to_pixel = color;
            }
            from_pixel++;
            to_pixel++;
        }
        from_row += spritemap->pitch;
        to_row += buffer->pitch;
    }
}

void DrawScaledTile(PixelBuffer* buffer, PixelBuffer* spritemap, TileRect tiles_to, TileRect sprite)
{
    Rect to_rect = ToRect(tiles_to);
    Rect from_rect = ToRect(sprite);

    float scale_factor = (float)fmax(sprite.w / (float)tiles_to.w, sprite.h / (float)tiles_to.h);


    for (int row=0; row < to_rect.w; row++)
    {
        for (int col=0; col < to_rect.h; col++)
        {
            Coord get = {(int)(col * scale_factor), (int)(row * scale_factor)};
            if (get.x < from_rect.w && get.y < from_rect.h)
            {
                uint8 color = GetPointFromSprite(spritemap, get, sprite);
                Coord draw_point = {to_rect.pos.x + col, to_rect.pos.y + row};
                DrawPoint(buffer, draw_point, color);

            }
        }
    }
}

void DrawSprite(PixelBuffer* buffer, PixelBuffer* spritemap, Coord to_pos, TileCoord sprite, uint8 transparent)
{
    DrawSprite(buffer, spritemap, to_pos, {sprite, 1, 1}, transparent);
}

void DrawSprite(PixelBuffer* buffer, PixelBuffer* spritemap, Coord to_pos, uint8 sprite, uint8 transparent)
{
    DrawSprite(buffer, spritemap, to_pos, SpriteToTileCoord(sprite), transparent);
}

void DrawTile(PixelBuffer* buffer, PixelBuffer* spritemap, TileCoord pos, TileRect sprite)
{
    DrawImage(buffer, spritemap, ToCoord(pos), ToRect(sprite));
}

void DrawTile(PixelBuffer* buffer, PixelBuffer* spritemap, TileCoord pos, TileCoord sprite)
{
    DrawTile(buffer, spritemap, pos, {sprite, 1, 1});
}

void DrawTile(PixelBuffer* buffer, PixelBuffer* spritemap, TileCoord pos, uint8 sprite)
{
    DrawTile(buffer, spritemap, pos, SpriteToTileCoord(sprite));
}

void CopySprite(PixelBuffer* to_buffer, PixelBuffer* from_buffer, TileCoord to_sprite, TileRect from_sprite)
{
    DrawTile(to_buffer, from_buffer, to_sprite, from_sprite);
}

void CopySprite(PixelBuffer* buffer, TileCoord to_sprite, TileRect from_sprite)
{
    CopySprite(buffer, buffer, to_sprite, from_sprite);
}

void DrawRect(PixelBuffer* buffer, Rect rect, uint8 color)
{
    rect = Trim(rect, {0, 0, buffer->w, buffer->h});
    uint8* pixel_row = GetPixel(buffer, rect.pos);
    for (int row=0; row < rect.h; row++)
    {
        memset(pixel_row, color, rect.w);
        pixel_row += buffer->pitch;
    }
}

void DrawRectAligned(PixelBuffer* buffer, TileRect rect, uint8 color)
{
    DrawRect(buffer, ToRect(rect), color);
}

void DrawClear(PixelBuffer* buffer, uint8 color)
{
    DrawRect(buffer, {0, 0, GAME_WIDTH, GAME_HEIGHT}, color);
}
