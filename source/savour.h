#ifndef SAVOUR_H
#define SAVOUR_H

#include "and_common.h"

#include "savour_platform.h"

struct image
{
    i32 Width;
    i32 Height;
    void *Pixels;
};

struct rect
{
    i32 X;
    i32 Y;
    i32 Width;
    i32 Height;
};


inline image GetImageFromPlatformImage(platform_image PlatformImage)
{
    image Result = {};
    
    Result.Width = PlatformImage.Width;
    Result.Height = PlatformImage.Height;
    Result.Pixels = PlatformImage.ImageData;

    return Result;
}

struct game_state
{
    image FontAtlas;
    b32 IsBilinear;

    u32 MapGlyphs[2048];
    i32 MapWidth;
    i32 MapHeight;

    i32 PlayerX;
    i32 PlayerY;

    f32 CameraOffsetX;
    f32 CameraOffsetY;
    f32 CameraZoom;
};

#endif
