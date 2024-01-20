#ifndef SAVOUR_H
#define SAVOUR_H

#include "and_common.h"

#include "savour_platform.h"

struct image
{
    u32 Width;
    u32 Height;
    void *Pixels;
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

    i32 PlayerX = 0;
    i32 PlayerY = 0;
};

#endif
