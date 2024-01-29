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
    // TODO: Maybe the visual of an entity should "palletized"
    u8 Glyph;
    // TODO: Store the 2 colors as 2 u32s
    vec3 ForegroundColor;
    vec3 BackgroundColor;
    
    vec3i P;
    
    // TODO: Flags in u8/16/32
    b32 IsBlocking;
    b32 IsOpaque;

    entity *Next;
};

// NOTE: Chunk 16x16x1
#define ChunkEntityCount 256
struct chunk
{
    vec3i P;
    
    entity *Entities[ChunkEntityCount];

    chunk *Next;
};

#define WorldEntityCount 1000000 //16384

struct game_state
{
    memory_arena RootArena;
    memory_arena WorldArena;
    
    font_atlas FontAtlas;
    b32 IsBilinear;

    // TODO: Need a hash table
    chunk *Chunks;
    vec3i ChunkDim;

    entity WorldEntities[WorldEntityCount];
    u32 NextEmptyEntityIndex;
    entity *EntityFreeList;

    entity Player;
    entity OtherEntity;

    vec2i TileDim;
    vec2i TileDimForTest;

    vec3i CameraCenterTile;
    vec2 CameraTileOffset;
    f32 CameraTileOffsetMax;

    f32 CameraZoomMin;
    f32 CameraZoomMax;
    f32 CameraZoomLogNeutral;
    f32 CameraZoomLogCurrent;
    b32 CameraZoomStartedBeforeNeutral;
    b32 CameraZoomIsInitial;
};

#endif
