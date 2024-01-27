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

struct font_atlas
{
    image Image;
    i32 AtlasWidth;
    i32 AtlasHeight;
    i32 GlyphPxWidth;
    i32 GlyphPxHeight;
};

struct entity
{
    u8 Glyph;
    // TODO: Store the 2 colors as 6 bytes
    vec3 ForegroundColor;
    vec3 BackgroundColor;
    
    vec3i Position;
    
    // TODO: Flags
    b32 IsBlocking;
    b32 IsOpaque;
};

struct entity_node
{
    entity E;
    b32 IsPopulated;
    // TODO: Doubly linked?
    entity_node *Next;
};

#define MapTableEntryCount 16384

struct game_state
{
    font_atlas FontAtlas;
    b32 IsBilinear;

    i32 MapWidth;
    i32 MapHeight;

    // TODO: Whether a floor is an entity is still pending
    // For now the first entity node in each map table entry is the floor
    entity_node MapTable[MapTableEntryCount];
    entity_node *EntityNodeFreeList;

    entity_node *PlayerEntityNode;

    f32 CameraOffsetX;
    f32 CameraOffsetY;
    f32 CameraZoom;
    b32 CameraInitialZoom;
};

#endif
