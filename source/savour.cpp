#include "and_common.h"
#include "and_math.h"
#include "and_linmath.h"

#include "savour_platform.h"
#include "savour.h"

#include <ctime>

u32
AlphaBlendBgFg(vec3 Bg, vec3 Fg, f32 Alpha)
{
    u8 R = ((u8) ((Bg.R * 255) * (1 - Alpha)) +
            (u8) ((Fg.R * 255) * (    Alpha)));
    u8 G = ((u8) ((Bg.G * 255) * (1 - Alpha)) +
            (u8) ((Fg.G * 255) * (    Alpha)));
    u8 B = ((u8) ((Bg.B * 255) * (1 - Alpha)) +
            (u8) ((Fg.B * 255) * (    Alpha)));
            
    u32 Result = ((R    << 24) |
                  (G    << 16) |
                  (B    << 8 ) |
                  (0xFF << 0));

    return Result;
}

u32
InterpolatePixel(u32 A, u32 B, f32 Ratio)
{
    u32 RMask = 0xFF000000;
    u32 GMask = 0x00FF0000;
    u32 BMask = 0x0000FF00;
    u32 AMask = 0x000000FF;

    u8 aR = (u8) ((A & RMask) >> 24);
    u8 aG = (u8) ((A & GMask) >> 16);
    u8 aB = (u8) ((A & BMask) >> 8 );
    u8 aA = (u8) ((A & AMask) >> 0 );

    u8 bR = (u8) ((B & RMask) >> 24);
    u8 bG = (u8) ((B & GMask) >> 16);
    u8 bB = (u8) ((B & BMask) >> 8 );
    u8 bA = (u8) ((B & AMask) >> 0 );

    u8 cR = (u8) (aR * (1.0f - Ratio)) + (u8) (bR * Ratio);
    u8 cG = (u8) (aG * (1.0f - Ratio)) + (u8) (bG * Ratio);
    u8 cB = (u8) (aB * (1.0f - Ratio)) + (u8) (bB * Ratio);
    u8 cA = (u8) (aA * (1.0f - Ratio)) + (u8) (bA * Ratio);

    u32 Result = ((cR << 24) |
                  (cG << 16) |
                  (cB <<  8) |
                  (cA <<  0));

    return Result;
}

void
BlitAlpha(image Source, rect SourceRect, image Dest, rect DestRect, vec3 Bg, vec3 Fg, b32 Bilinear)
{
    Assert(SourceRect.X >= 0);
    Assert(SourceRect.X < Source.Width);
    Assert(SourceRect.X + SourceRect.Width <= Source.Width);
    Assert(SourceRect.Y >= 0);
    Assert(SourceRect.Y < Source.Height);
    Assert(SourceRect.Y + SourceRect.Height <= Source.Height);

    u32 *SourcePixels = (u32 *) Source.Pixels;
    u32 *DestPixels = (u32 *) Dest.Pixels;

    i32 DestRectMinX = DestRect.X;
    if (DestRectMinX > Dest.Width)
    {
        return;
    }
    else if (DestRectMinX < 0)
    {
        DestRectMinX = 0;
    }
    i32 DestRectMinY = DestRect.Y;
    if (DestRectMinY > Dest.Height)
    {
        return;
    }
    else if (DestRectMinY < 0)
    {
        DestRectMinY = 0;
    }

    i32 DestRectMaxX = DestRect.X + DestRect.Width;
    if (DestRectMaxX < 0)
    {
        return;
    }
    else if (DestRectMaxX > Dest.Width)
    {
        DestRectMaxX = Dest.Width;
    }
    i32 DestRectMaxY = DestRect.Y + DestRect.Height;
    if (DestRectMaxY < 0)
    {
        return;
    }
    else if (DestRectMaxY > Dest.Height)
    {
        DestRectMaxY = Dest.Height;
    }
    
    for (i32 RowI = DestRectMinY;
         RowI < DestRectMaxY;
         ++RowI)
    {
        f32 DestRectYRatio = (f32) (RowI - DestRect.Y) / (f32) (DestRect.Height);
        for (i32 ColumnI = DestRectMinX;
             ColumnI < DestRectMaxX;
             ++ColumnI)
        {
            u32 *DestPixel = DestPixels + RowI * Dest.Width + ColumnI;
            
            f32 DestRectXRatio = (f32) (ColumnI - DestRect.X) / (f32) (DestRect.Width);

            u32 ResultingPixel;
            
            u32 SourceX = SourceRect.X + (u32) (DestRectXRatio * SourceRect.Width);
            u32 SourceY = SourceRect.Y + (u32) (DestRectYRatio * SourceRect.Height);
            u32 *SourcePixel = SourcePixels + SourceY * Source.Width + SourceX;
            // printf("D[%u,%u](%0.3f,%0.3f)->S[%u,%u]\n", ColumnI, RowI, DestRectXRatio, DestRectYRatio, SourceX, SourceY);
            
            ResultingPixel = *SourcePixel;
            
            #if 0
            if (!Bilinear)
            {
                // Nearest neighbor
                // TODO: Truncate or round?
                // When I was rounding, everything was shifted half pixel to the left
                u32 SourceX = SourceRect.X + (u32) (DestRectXRatio * SourceRect.Width);
                u32 SourceY = SourceRect.Y + (u32) (DestRectYRatio * SourceRect.Height);
                u32 *SourcePixel = SourcePixels + SourceY * Source.Width + SourceX;
                // printf("D[%u,%u](%0.3f,%0.3f)->S[%u,%u]\n", ColumnI, RowI, DestRectXRatio, DestRectYRatio, SourceX, SourceY);
            
                ResultingPixel = *SourcePixel;
            }
            else
            {
                // Bilinear interpolation
                // TODO: Not sure if this even does anything lol. Need to find a way to test this somehow
                // TODO: We should not sample outside of the source rectangle
                f32 FloorRectX = FloorF(DestRectXRatio * SourceRect.Width);
                f32 FloorRectY = FloorF(DestRectYRatio * SourceRect.Height);
                f32 RatioX = FloorRectX - (f32) ((u32) FloorRectX);
                f32 RatioY = FloorRectY - (f32) ((u32) FloorRectY);
            
                u32 FloorX = SourceRect.X + (u32) FloorRectX;
                u32 FloorY = SourceRect.Y + (u32) FloorRectY;
                u32 CeilingX = SourceRect.X + (u32) CeilingF(DestRectXRatio * SourceRect.Width);
                u32 CeilingY = SourceRect.Y + (u32) CeilingF(DestRectYRatio * SourceRect.Height);

                u32 *Pixel00 = SourcePixels + FloorY * Source.Width + FloorX;
                u32 *Pixel01 = SourcePixels + FloorY * Source.Width + CeilingX;
                u32 *Pixel10 = SourcePixels + CeilingY * Source.Width + FloorX;
                u32 *Pixel11 = SourcePixels + CeilingY * Source.Width + CeilingX;

                ResultingPixel = InterpolatePixel(InterpolatePixel(*Pixel00, *Pixel01, RatioX),
                                                  InterpolatePixel(*Pixel10, *Pixel11, RatioX),
                                                  RatioY);
            }
            #endif
            
            f32 SourceAlpha = (u8) ResultingPixel / 255.0f;

            *DestPixel = AlphaBlendBgFg(Bg, Fg, SourceAlpha);
        }
    }

    return;
}

void
GameUpdateAndRender(game_input *GameInput, game_memory *GameMemory, platform_image *OffscreenBuffer, b32 *GameShouldQuit)
{
    game_state *GameState = (game_state *) GameMemory->Storage;

    if (!GameMemory->IsInitialized)
    {
        // TODO: Temporary
        srand((u32)time(NULL)); 

        GameState->FontAtlas = GetImageFromPlatformImage(Platform_LoadBMP("resources/font.bmp"));

        GameState->MapWidth = 64;
        GameState->MapHeight = 32;
        for (u32 MapGlyphI = 0;
             MapGlyphI < 2048;
             ++MapGlyphI)
        {
            i32 X = MapGlyphI % GameState->MapWidth;
            i32 Y = MapGlyphI / GameState->MapWidth;

            if (X == 0 || X == (GameState->MapWidth - 1) ||
                Y == 0 || Y == (GameState->MapHeight - 1))
            {
                GameState->MapGlyphs[MapGlyphI] = '#';
            }
            else
            {
                GameState->MapGlyphs[MapGlyphI] = (((rand() % 2) == 0) ? '`' : ',');
            }
        }

        GameState->CameraZoom = 1.0f;
        
        GameMemory->IsInitialized = true;
    }

    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_ESCAPE))
    {
        *GameShouldQuit = true;
    }
    
    i32 ScreenGlyphWidth = 48;
    i32 ScreenGlyphHeight = 72;
    i32 AtlasGlyphWidth = 48;
    i32 AtlasGlyphHeight = 72;

    rect SourceRect = {};
    SourceRect.X = 48;
    SourceRect.Y = 0;
    SourceRect.Width = AtlasGlyphWidth;
    SourceRect.Height = AtlasGlyphHeight;
    rect DestRect = {};
    DestRect.X = 0;
    DestRect.Y = 0;
    DestRect.Width = (i32) (ScreenGlyphWidth * GameState->CameraZoom);
    DestRect.Height = (i32) (ScreenGlyphHeight * GameState->CameraZoom);

    image ScreenImage = {};
    ScreenImage.Pixels = OffscreenBuffer->ImageData;
    ScreenImage.Width = OffscreenBuffer->Width;
    ScreenImage.Height = OffscreenBuffer->Height;

    for (i32 MapGlyphI = 0;
         MapGlyphI < 2048;
         ++MapGlyphI)
    {
        DestRect.X = (i32) (((MapGlyphI % GameState->MapWidth) * ScreenGlyphWidth - (i32) GameState->CameraOffsetX) * GameState->CameraZoom);
        DestRect.Y = (i32) (((MapGlyphI / GameState->MapWidth) * ScreenGlyphHeight - (i32) GameState->CameraOffsetY) * GameState->CameraZoom);

        if (GameState->MapGlyphs[MapGlyphI] == '#')
        {
            SourceRect.X = 144;
            SourceRect.Y = 144;
        }
        else if (GameState->MapGlyphs[MapGlyphI] == '`')
        {
            SourceRect.X = 0;
            SourceRect.Y = 432;
        }
        else
        {
            SourceRect.X = 576;
            SourceRect.Y = 144;
        }

        if (MapGlyphI == 0 && Platform_KeyIsDown(GameInput, SDL_SCANCODE_B))
        {
            Breakpoint;
        }

        BlitAlpha(GameState->FontAtlas, SourceRect, ScreenImage, DestRect, Vec3(1), Vec3(0), false);
    }

    f32 PixelsPerSecond = 200.0f;
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_H))
    {
        GameState->CameraOffsetX -= PixelsPerSecond * GameInput->DeltaTime;
    }
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_L))
    {
        GameState->CameraOffsetX += PixelsPerSecond * GameInput->DeltaTime;
    }
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_K))
    {
        GameState->CameraOffsetY -= PixelsPerSecond * GameInput->DeltaTime;
    }
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_J))
    {
        GameState->CameraOffsetY += PixelsPerSecond * GameInput->DeltaTime;
    }

    if (Platform_MouseButtonIsDown(GameInput, MouseButton_Middle))
    {
        GameState->CameraOffsetX -= GameInput->MouseLogicalDeltaX / GameState->CameraZoom;
        GameState->CameraOffsetY -= GameInput->MouseLogicalDeltaY / GameState->CameraZoom;
    }

    f32 ZoomPerSecond = 1.0f;
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_PAGEUP))
    {
        GameState->CameraZoom += ZoomPerSecond * GameInput->DeltaTime;
    }
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_PAGEDOWN))
    {
        GameState->CameraZoom -= ZoomPerSecond * GameInput->DeltaTime;
    }
}
