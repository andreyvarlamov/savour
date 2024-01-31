#include "and_common.h"
#include "and_math.h"
#include "and_linmath.h"
#include "and_random.h"

#include "savour_platform.h"
#include "savour.h"

#include <ctime>
#include <climits>

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

inline void
BlitAlphaInvY(image Source, rect SourceRect, image Dest, rect DestRect, vec3 Bg, vec3 Fg, b32 Bilinear)
{
    DestRect.Y = -(DestRect.Y - Dest.Height) - DestRect.Height;
    BlitAlpha(Source, SourceRect, Dest, DestRect, Bg, Fg, Bilinear);
}

void
RenderGlyph(font_atlas FontAtlas, u8 Glyph, image ScreenImage, rect DestRect, vec3 ForegroundColor, vec3 BackgroundColor)
{
    i32 GlyphX = (i32) Glyph % FontAtlas.AtlasWidth;
    i32 GlyphY = (i32) Glyph / FontAtlas.AtlasWidth;
    
    rect SourceRect = {};
    SourceRect.X = GlyphX * FontAtlas.GlyphPxWidth;
    SourceRect.Y = GlyphY * FontAtlas.GlyphPxHeight;
    SourceRect.Width = FontAtlas.GlyphPxWidth;
    SourceRect.Height = FontAtlas.GlyphPxHeight;
    
    BlitAlphaInvY(FontAtlas.Image, SourceRect, ScreenImage, DestRect, ForegroundColor, BackgroundColor, false);
}

u32
GetMapIndex(vec3i Position, i32 MapWidth)
{
    u32 Result = Position.X + MapWidth * Position.Y;
    return Result;
}

#if 0
entity_node *
GetMapTopEntityNodeInPosition(entity_node *MapTable, i32 MapWidth, vec3i Position)
{
    // NOTE: Return the top *populated* node
    
    entity_node *Result = MapTable + GetMapIndex(Position, MapWidth);
    
    Assert(Result->IsPopulated);
    
    while (Result->Next)
    {
        Result = Result->Next;
    }
    
    return Result;
}

entity_node *
GetFreeEntityNodeInMapTable(game_state *GameState)
{
    entity_node *Result = 0;
    
    if (GameState->EntityNodeFreeList)
    {
        Result = GameState->EntityNodeFreeList;

        GameState->EntityNodeFreeList = GameState->EntityNodeFreeList->Next;

        Result->Next = 0;
    }
    else
    {
        for (u32 EntityNodeI = GameState->MapWidth * GameState->MapHeight;
             EntityNodeI < MapTableEntryCount;
             ++EntityNodeI)
        {
            entity_node *EntityNode = GameState->MapTable + EntityNodeI;

            if (!EntityNode->IsPopulated)
            {
                Result = EntityNode;
                break;
            }
        }
    }

    Assert(Result);

    return Result;
}

entity_node *
GetEntityParentNode(entity_node *MapTable, i32 MapWidth, entity_node *ChildEntityNode)
{
    entity_node *Result = MapTable + GetMapIndex(ChildEntityNode->E.Position, MapWidth);

    if (Result == ChildEntityNode)
    {
        return Result;
    }

    while (Result->Next)
    {
        if (Result->Next == ChildEntityNode)
        {
            return Result;
        }
        
        Result = Result->Next;
    }

    InvalidCodePath;
    
    return 0;
}
#endif

inline i32
GetChunkFromTile(i32 Tile, i32 ChunkDim)
{
    Assert(INT_MIN + ChunkDim <= Tile);
    
    if (Tile < 0)
    {
        Tile -= ChunkDim - 1;
    }

    i32 Result = Tile / ChunkDim;
    return Result;
}

inline i32
GetLeftmostTileFromChunk(i32 Chunk, i32 ChunkDim)
{
    i32 Tile = Chunk * ChunkDim;
    return Tile;
}

inline vec3i
GetChunkPFromTileP(vec3i TileP, vec3i ChunkDim)
{
    vec3i ChunkP = Vec3I(GetChunkFromTile(TileP.X, ChunkDim.X),
                         GetChunkFromTile(TileP.Y, ChunkDim.Y),
                         GetChunkFromTile(TileP.Z, ChunkDim.Z));
    return ChunkP;
}

inline vec3i
GetLeftmostTilePFromChunkP(vec3i ChunkP, vec3i ChunkDim)
{
    vec3i TileP = Vec3I(GetLeftmostTileFromChunk(ChunkP.X, ChunkDim.X),
                        GetLeftmostTileFromChunk(ChunkP.Y, ChunkDim.Y),
                        GetLeftmostTileFromChunk(ChunkP.Z, ChunkDim.Z));
    return TileP;
}

inline i32
GetTileFromPixel(i32 Pixel, i32 TileDim)
{
    
}

inline i32
GetPixelFromTile(i32 Tile, i32 TileDim)
{
    i32 Pixel = Tile * TileDim;
    return Pixel;
}

inline f32
ExponentialInterpolation(f32 Min, f32 Max, f32 T)
{
    f32 LogMin = log(Min);
    f32 LogMax = log(Max);
    f32 Lerp = LogMin + (LogMax - LogMin) * T;
    
    f32 Result = exp(Lerp);
    return Result;
}

inline f32
ExponentialInterpolationWhereIs(f32 Min, f32 Max, f32 V)
{
    f32 LogMin = log(Min);
    f32 LogMax = log(Max);
    f32 LogV = log(V);

    f32 Result;
    
    if (LogV >= LogMax)
    {
        Result = 1.0f;
    }
    else if (LogV <= LogMin)
    {
        Result = 0.0f;
    }
    else
    {
        Result = (LogV - LogMin) / (LogMax - LogMin);
    }
    
    return Result;
}

void
CalculateChunkRectInCameraView(i32 ScreenWidth, i32 ScreenHeight, vec2i TileDim, vec3i CameraPosition, vec3i ChunkDim,
                               vec3i *Out_ChunkMin, vec3i *Out_ChunkMax)
{
    f32 ScreenHalfWidthInTiles = ((f32) ScreenWidth * 0.5f) / (f32) TileDim.X;
    f32 ScreenHalfHeightInTiles = ((f32) ScreenHeight * 0.5f) / (f32) TileDim.Y;

    i32 TileMinX = CameraPosition.X + (i32) FloorF(-ScreenHalfWidthInTiles);
    i32 TileMaxX = CameraPosition.X + (i32) CeilingF(ScreenHalfWidthInTiles);
    i32 TileMinY = CameraPosition.Y + (i32) FloorF(-ScreenHalfHeightInTiles);
    i32 TileMaxY = CameraPosition.Y + (i32) CeilingF(ScreenHalfHeightInTiles);

    *Out_ChunkMin = GetChunkPFromTileP(Vec3I(TileMinX, TileMinY, CameraPosition.Z), ChunkDim);
    *Out_ChunkMax = GetChunkPFromTileP(Vec3I(TileMaxX, TileMaxY, CameraPosition.Z), ChunkDim);

    #if 0
    printf("TileMin(%d, %d); TileMax(%d, %d); ", TileMinX, TileMinY, TileMaxX, TileMaxY);
    printf("TileDim(%d, %d); ", TileDim.X, TileDim.Y);
    printf("ScreenHalfWidthInTiles(%0.3f, %0.3f); ", ScreenHalfWidthInTiles, ScreenHalfHeightInTiles);
    printf("ChunkMin(%d, %d); ChunkMax(%d, %d)\n", ChunkMin.X, ChunkMin.Y, ChunkMax.X, ChunkMax.Y);
    #endif
}

entity *
GetFreeEntity(game_state *GameState)
{
    entity *Result;
    
    if (GameState->EntityFreeList)
    {
        Result = GameState->EntityFreeList;
        GameState->EntityFreeList = GameState->EntityFreeList->Next;
        Result->Next = 0;
    }
    else
    {
        Assert(GameState->NextEmptyEntityIndex < WorldEntityCount);
        Result = GameState->WorldEntities + GameState->NextEmptyEntityIndex++;
    }

    Assert(Result);

    return Result;
}

void
DebugMap(memory_arena *TransientArena, i32 MinX, i32 MinY, i32 MaxX, i32 MaxY,
         platform_image *Out_ContinentalPerlin, platform_image *Out_TerrainPerlin, platform_image *Out_MapImage)
{
    u32 Width = MaxX - MinX + 1;
    u32 Height = MaxY - MinY + 1;
    Out_ContinentalPerlin->Width = Width;
    Out_ContinentalPerlin->Height = Height;
    Out_TerrainPerlin->Width = Width;
    Out_TerrainPerlin->Height = Height;
    Out_MapImage->Width = Width;
    Out_MapImage->Height = Height;

    Out_ContinentalPerlin->ImageData = (void *) MemoryArena_PushArray(TransientArena, Width * Height, u32);
    Out_TerrainPerlin->ImageData = (void *) MemoryArena_PushArray(TransientArena, Width * Height, u32);
    Out_MapImage->ImageData = (void *) MemoryArena_PushArray(TransientArena, Width * Height, u32);

    u32 *ContinentalP = (u32 *) Out_ContinentalPerlin->ImageData;
    u32 *TerrainP = (u32 *) Out_TerrainPerlin->ImageData;
    u32 *MapP = (u32 *) Out_MapImage->ImageData;
    for (i32 Y = MinY;
         Y <= MaxY;
         ++Y)
    {
        for (i32 X = MinX;
             X <= MaxX;
             ++X)
        {
            // f32 ContinentalIntensity = PerlinSampleOctaves(X / 256.0f, Y / 256.0f, 1.3f, 0.3f, 4, 100);
            f32 ContinentalIntensity = PerlinSampleOctaves(X / 256.0f, Y / 256.0f, 2.1f, 0.3f, 4, 100) * 2.0f - 0.3f;
            ContinentalIntensity = PerlinNormalize(ContinentalIntensity);

            u8 ContinentalByte = (u8) (ContinentalIntensity * 255.0f);
            *ContinentalP++ = (ContinentalByte << 24 |
                           ContinentalByte << 16 |
                           ContinentalByte << 8  |
                           255);


            f32 Intensity = PerlinSampleOctaves(X / 32.0f, Y / 32.0f, 1.8f, 0.5f, 6, 101);
            Intensity = PerlinNormalize(Intensity);

            u8 TerrainByte = (u8) (Intensity * 255.0f);
            *TerrainP++ = (TerrainByte << 24 |
                           TerrainByte << 16 |
                           TerrainByte << 8  |
                           255);

            if (ContinentalIntensity < 0.5f || Intensity <= 0.4f)
            {
                // NOTE: Water
                *MapP = 0x0000FFFF;
            }
            else if (Intensity >= 0.6f)
            {
                // NOTE: Mountain
                *MapP = 0xAAAAAAFF;
            }
            else
            {
                // NOTE: Grass
                *MapP = 0x00FF00FF;
            }

            if (X == 0 && Y == 0)
            {
                *MapP = 0x000000FF;
            }

            *MapP++;
        }
    }
}

void
GenerateChunkTerrain(vec3i ChunkP, game_state *GameState, memory_arena *WorldArena)
{
    local_persist i32 CurrentIndex = 0;
    printf("Gen ch#%d - P(%d,%d,%d)... ", CurrentIndex++, ChunkP.X, ChunkP.Y, ChunkP.Z);
    
    chunk *Chunk = MemoryArena_PushStruct(WorldArena, chunk);
    Chunk->P = ChunkP;
    Chunk->Next = GameState->Chunks;
    GameState->Chunks = Chunk;
    
    for (i32 I = 0;
         I < ChunkEntityCount;
         ++I)
    {
        entity *TopEntity = GetFreeEntity(GameState);

        i32 X = I % GameState->ChunkDim.X;
        i32 Y = I / GameState->ChunkDim.Y;
        i32 Z = ChunkP.Z;
        
        vec3i Position = GetLeftmostTilePFromChunkP(ChunkP, GameState->ChunkDim) + Vec3I(X, Y, Z);

        f32 ContinentalIntensity = PerlinSampleOctaves(Position.X / 256.0f, Position.Y / 256.0f, 1.3f, 0.3f, 4, 100);
        ContinentalIntensity = PerlinNormalize(ContinentalIntensity);

        f32 Intensity = PerlinSampleOctaves(Position.X / 32.0f, Position.Y / 32.0f, 1.8f, 0.5f, 6, 101);
        Intensity = PerlinNormalize(Intensity);

        // NOTE: Grass
        TopEntity->Glyph = (((rand() % 2) ==  0) ? 176 : 177);
        TopEntity->ForegroundColor = Vec3(0.4f, 0.7f, 0.4f);
        TopEntity->BackgroundColor = Vec3(0.3f, 0.6f, 0.4f);
        TopEntity->P = Position;
        TopEntity->IsBlocking = false;
        TopEntity->IsOpaque = false;

        if (ContinentalIntensity < 0.5f || Intensity <= 0.4f)
        {
            // NOTE: Water
            entity *OldTop = TopEntity;
            TopEntity = GetFreeEntity(GameState);
            TopEntity->Next = OldTop;
            
            TopEntity->Glyph = (((rand() % 2) ==  0) ? 247 : 126);
            TopEntity->ForegroundColor = Vec3(0.3f, 0.3f, 0.8f);
            TopEntity->BackgroundColor = Vec3(0.2f, 0.2f, 0.6f);
            TopEntity->P = Position;
            TopEntity->IsBlocking = false;
            TopEntity->IsOpaque = false;
        }
        else if (Intensity >= 0.6f)
        {
            // NOTE: Mountain
            entity *OldTop = TopEntity;
            TopEntity = GetFreeEntity(GameState);
            TopEntity->Next = OldTop;
            
            TopEntity->Glyph = (((rand() % 2) == 0) ? '#' : '%');
            TopEntity->ForegroundColor = Vec3(0.42f);
            TopEntity->BackgroundColor = Vec3(0.4f);
            TopEntity->P = Position;
            TopEntity->IsBlocking = true;
            TopEntity->IsOpaque = true;
        }
        else
        {
            // NOTE: Grass is always there
        }

        Chunk->Entities[I] = TopEntity;
    }
    printf("Done. E:%d. WA:%zuKB/%zuKB \n", GameState->NextEmptyEntityIndex, WorldArena->Used / 1024, WorldArena->Size / 1024);
}

void
GameUpdateAndRender(game_input *GameInput, game_memory *GameMemory, platform_image *OffscreenBuffer, b32 *GameShouldQuit)
{
    game_state *GameState = (game_state *) GameMemory->Storage;

    if (!GameMemory->IsInitialized)
    {
        printf("Need %zu MB for entities.\n", (WorldEntityCount * sizeof(entity) / 1024) / 1024);
        
        // NOTE: Initialize memory arenas
        u8 *RootArenaBase = (u8 *) GameMemory->Storage + sizeof(game_state);
        size_t RootArenaSize = GameMemory->StorageSize - sizeof(game_state);
        GameState->RootArena = MemoryArena(RootArenaBase, RootArenaSize);

        GameState->TransientArena = MemoryArenaNested(&GameState->RootArena, Megabytes(64));
        GameState->WorldArena = MemoryArenaNested(&GameState->RootArena, Megabytes(4));

        // Generate and save map preview
        platform_image ContinentalPerlin;
        platform_image TerrainPerlin;
        platform_image MapImage;
        DebugMap(&GameState->TransientArena, -512, -512, 512, 512,
                         &ContinentalPerlin, &TerrainPerlin, &MapImage);
        Platform_SaveRGBA_BMP(&ContinentalPerlin, "continental", true);
        Platform_SaveRGBA_BMP(&TerrainPerlin, "terrain", true);
        Platform_SaveRGBA_BMP(&MapImage, "map", true);
        
        // TODO: Temporary
        srand((u32)time(NULL));

        // NOTE: Initialize font atas;
        GameState->FontAtlas.Image = GetImageFromPlatformImage(Platform_LoadBMP("resources/font.bmp"));
        GameState->FontAtlas.AtlasWidth = 16;
        GameState->FontAtlas.AtlasHeight = 16;
        GameState->FontAtlas.GlyphPxWidth = 48;
        GameState->FontAtlas.GlyphPxHeight = 72;

        // NOTE: Initialize camera
        GameState->CameraZoomMin = 0.2f;
        GameState->CameraZoomMax = 10.0f;
        GameState->CameraZoomLogNeutral = ExponentialInterpolationWhereIs(GameState->CameraZoomMin, GameState->CameraZoomMax, 1.0f);
        GameState->CameraZoomLogCurrent = GameState->CameraZoomLogNeutral;
        // GameState->CameraZoomIsInitial = true;
        GameState->CameraTileOffsetMax = 32.0f;
        GameState->CameraCenterTile = Vec3I();

        f32 CameraZoomCurrent = ExponentialInterpolation(GameState->CameraZoomMin, GameState->CameraZoomMax, GameState->CameraZoomLogCurrent);
        GameState->TileDim = Vec2I(GameState->FontAtlas.GlyphPxWidth, GameState->FontAtlas.GlyphPxHeight) * CameraZoomCurrent;
        GameState->TileDimForTest = Vec2I(GameState->FontAtlas.GlyphPxWidth, GameState->FontAtlas.GlyphPxHeight) * ExponentialInterpolation(GameState->CameraZoomMin, GameState->CameraZoomMax, 0.0f);

        // NOTE: Initialize first chunks
        GameState->ChunkDim = Vec3I(16,16,1);

        vec3i ChunkMin, ChunkMax;
        CalculateChunkRectInCameraView(OffscreenBuffer->Width, OffscreenBuffer->Height,
                                       GameState->TileDimForTest, GameState->CameraCenterTile, GameState->ChunkDim,
                                       &ChunkMin, &ChunkMax);
        printf("ChunkMin(%d, %d); ChunkMax(%d, %d)\n", ChunkMin.X, ChunkMin.Y, ChunkMax.X, ChunkMax.Y);

        for (i32 ChunkY = ChunkMin.Y;
             ChunkY <= ChunkMax.Y;
             ++ChunkY)
        {
            for (i32 ChunkX = ChunkMin.X;
                 ChunkX <= ChunkMax.X;
                 ++ChunkX)
            {
                GenerateChunkTerrain(Vec3I(ChunkX, ChunkY, GameState->CameraCenterTile.Z), GameState, &GameState->WorldArena);
            }
        }

        // NOTE: Create player entity
        {
            GameState->Player.P = GameState->CameraCenterTile;
            GameState->Player.Glyph = '@';
            GameState->Player.ForegroundColor = Vec3(0);
            GameState->Player.BackgroundColor = Vec3(0,0,1);
            GameState->Player.IsBlocking = true;
            GameState->Player.IsOpaque = false;
        }

        {
            GameState->OtherEntity.P = GameState->CameraCenterTile + Vec3I(3, 3, 0);
            GameState->OtherEntity.Glyph = 'A';
            GameState->OtherEntity.ForegroundColor = Vec3(0);
            GameState->OtherEntity.BackgroundColor = Vec3(0,1,0);
            GameState->OtherEntity.IsBlocking = true;
            GameState->OtherEntity.IsOpaque = false;
        }

        GameMemory->IsInitialized = true;
    } // NOTE: DONE INIT

    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_ESCAPE))
    {
        *GameShouldQuit = true;
    }
    
    b32 PlayerMoved = false;
    vec3i NewPlayerPosition = GameState->Player.P;
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_A))
    {
        NewPlayerPosition.X--;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_D))
    {
        NewPlayerPosition.X++;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_W))
    { 
        NewPlayerPosition.Y++;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_S))
    {
        NewPlayerPosition.Y--;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_Q))
    {
        NewPlayerPosition.X--;
        NewPlayerPosition.Y++;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_E))
    {
        NewPlayerPosition.X++;
        NewPlayerPosition.Y++;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_Z))
    {
        NewPlayerPosition.X--;
        NewPlayerPosition.Y--;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_C))
    {
        NewPlayerPosition.X++;
        NewPlayerPosition.Y--;
        PlayerMoved = true;
    }

    GameState->Player.P = NewPlayerPosition;
    GameState->CameraCenterTile = GameState->Player.P;
    if (PlayerMoved)
    {
        // printf("%d,%d,%d\n", GameState->Player.P.X, GameState->Player.P.Y, GameState->Player.P.Z);
    }

    // NOTE: Test collisions
    #if 0
    if (PlayerMoved)
    {
        entity_node *TestEntityNode = GameState->MapTable + GetMapIndex(NewPlayerPosition, GameState->MapWidth);

        if (TestEntityNode->IsPopulated)
        {
            b32 FoundBlocking = false;
            entity_node *CurrentEntityNode = TestEntityNode;
            while(CurrentEntityNode)
            {
                if (CurrentEntityNode->E.IsBlocking)
                {
                    PlayerMoved = false;
                    break;
                }

                CurrentEntityNode = CurrentEntityNode->Next;
            }
        }
        else
        {
            PlayerMoved = false;
        }
    }
    #endif

    f32 LogZoomPerSecond = 1.0f;
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_PAGEUP))
    {
        if (Platform_KeyJustPressed(GameInput, SDL_SCANCODE_PAGEUP))
        {
            GameState->CameraZoomStartedBeforeNeutral = (GameState->CameraZoomLogCurrent < GameState->CameraZoomLogNeutral);
        }
        
        GameState->CameraZoomLogCurrent += LogZoomPerSecond * GameInput->DeltaTime;
        
        if (GameState->CameraZoomStartedBeforeNeutral && GameState->CameraZoomLogCurrent > GameState->CameraZoomLogNeutral)
        {
            GameState->CameraZoomLogCurrent = GameState->CameraZoomLogNeutral;
        }
        else if (GameState->CameraZoomLogCurrent > 1.0f)
        {
            GameState->CameraZoomLogCurrent = 1.0f;
        }
        
        GameState->CameraZoomIsInitial = false;
    }
    else if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_PAGEDOWN))
    {
        f32 ZoomBefore = 0.0f;
        if (Platform_KeyJustPressed(GameInput, SDL_SCANCODE_PAGEDOWN))
        {
            GameState->CameraZoomStartedBeforeNeutral = (GameState->CameraZoomLogCurrent > GameState->CameraZoomLogNeutral);
        }

        ZoomBefore = GameState->CameraZoomLogCurrent;
        GameState->CameraZoomLogCurrent -= LogZoomPerSecond * GameInput->DeltaTime;
        
        if (GameState->CameraZoomStartedBeforeNeutral && GameState->CameraZoomLogCurrent < GameState->CameraZoomLogNeutral)
        {
            GameState->CameraZoomLogCurrent = GameState->CameraZoomLogNeutral;
        }
        else if (GameState->CameraZoomLogCurrent < 0.0f)
        {
            GameState->CameraZoomLogCurrent = 0.0f;
        }

        GameState->CameraZoomIsInitial = false;
    }

    if (PlayerMoved && GameState->CameraZoomIsInitial)
    {
        GameState->CameraZoomLogCurrent -= 0.2f;
        if (GameState->CameraZoomLogCurrent < GameState->CameraZoomLogNeutral + 0.05f)
        {
            GameState->CameraZoomLogCurrent = GameState->CameraZoomLogNeutral;
            GameState->CameraZoomIsInitial = false;
        }
    }

    f32 CameraZoomCurrent = ExponentialInterpolation(GameState->CameraZoomMin, GameState->CameraZoomMax, GameState->CameraZoomLogCurrent);
    GameState->TileDim = Vec2I(GameState->FontAtlas.GlyphPxWidth, GameState->FontAtlas.GlyphPxHeight) * CameraZoomCurrent;

    if (Platform_MouseButtonIsDown(GameInput, MouseButton_Middle))
    {
        GameState->CameraTileOffset.X += GameInput->MouseLogicalDeltaX / (f32) GameState->TileDim.X;
        GameState->CameraTileOffset.Y -= GameInput->MouseLogicalDeltaY / (f32) GameState->TileDim.Y;
        GameState->CameraTileOffset = VecClamp(GameState->CameraTileOffset, GameState->CameraTileOffsetMax);
    }
    
    if (PlayerMoved)
    {
        GameState->CameraTileOffset.X = 0.0f;
        GameState->CameraTileOffset.Y = 0.0f;
    }

    vec3i ChunkMin, ChunkMax;
    CalculateChunkRectInCameraView(OffscreenBuffer->Width, OffscreenBuffer->Height,
                                   GameState->TileDimForTest, GameState->CameraCenterTile, GameState->ChunkDim,
                                   &ChunkMin, &ChunkMax);
    // printf("ChunkMin(%d, %d); ChunkMax(%d, %d)\n", ChunkMin.X, ChunkMin.Y, ChunkMax.X, ChunkMax.Y);

    // NOTE: Initialize chunks that haven't been yet
    for (i32 ChunkY = ChunkMin.Y;
         ChunkY <= ChunkMax.Y;
         ++ChunkY)
    {
        for (i32 ChunkX = ChunkMin.X;
             ChunkX <= ChunkMax.X;
             ++ChunkX)
        {
            vec3i RequestedP = Vec3I(ChunkX, ChunkY, GameState->CameraCenterTile.Z);
            chunk *SearchChunk = GameState->Chunks;
            while (SearchChunk)
            {
                if (Vec3IAreEqual(SearchChunk->P, RequestedP))
                {
                    break;
                }
                SearchChunk = SearchChunk->Next;
            }
            if (!SearchChunk)
            {
                GenerateChunkTerrain(Vec3I(ChunkX, ChunkY, 0), GameState, &GameState->WorldArena);
            }
        }
    }
    
    // printf("TileDim(%d,%d); CameraTileOffset(%0.5f,%0.5f)\n", GameState->TileDim.X, GameState->TileDim.Y, GameState->CameraTileOffset.X, GameState->CameraTileOffset.Y);

    //
    // NOTE: RENDER
    //
    
    image ScreenImage = {};
    ScreenImage.Pixels = OffscreenBuffer->ImageData;
    ScreenImage.Width = OffscreenBuffer->Width;
    ScreenImage.Height = OffscreenBuffer->Height;
    
    vec2i TileHalfDim = GameState->TileDim * 0.5f;
    vec2i ScreenHalfDim = Vec2I(ScreenImage.Width, ScreenImage.Height) * 0.5f;
    vec2i CameraPxOffset = Vec2I(GameState->CameraTileOffset * Vec2(GameState->TileDim));
    vec2i AllCameraOffsets = ScreenHalfDim - TileHalfDim + CameraPxOffset;

    rect DestRect = {};
    DestRect.Width = GameState->TileDim.X;
    DestRect.Height = GameState->TileDim.Y;

    for (i32 ChunkY = ChunkMin.Y;
         ChunkY <= ChunkMax.Y;
         ++ChunkY)
    {
        for (i32 ChunkX = ChunkMin.X;
             ChunkX <= ChunkMax.X;
             ++ChunkX)
        {
    // for (i32 ChunkY = -100;
    //      ChunkY <= 100;
    //      ++ChunkY)
    // {
    //     for (i32 ChunkX = -100;
    //          ChunkX <= 100;
    //          ++ChunkX)
    //     {
            vec3i RequestedP = Vec3I(ChunkX, ChunkY, GameState->CameraCenterTile.Z);
            chunk *Chunk = GameState->Chunks;
            while (Chunk)
            {
                if (Vec3IAreEqual(Chunk->P, RequestedP))
                {
                    break;
                }
                Chunk = Chunk->Next;
            }
            if (Chunk)
            {
                for (u32 ChunkEntityI = 0;
                     ChunkEntityI < ChunkEntityCount;
                     ++ChunkEntityI)
                {
                    entity *TopEntity = Chunk->Entities[ChunkEntityI];

                    vec2i EntityRelPxP = (Vec2I(TopEntity->P) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
                    DestRect.X = EntityRelPxP.X;
                    DestRect.Y = EntityRelPxP.Y;
                    RenderGlyph(GameState->FontAtlas, TopEntity->Glyph, ScreenImage, DestRect, TopEntity->BackgroundColor, TopEntity->ForegroundColor);
                }
            }
            else
            {
                Noop;
            }
        }
    }
    
    entity *AdditionalEntities[] = { &GameState->Player, &GameState->OtherEntity };
    for (u32 I = 0;
         I < ArrayCount(AdditionalEntities);
         ++I)
    {
        entity *Entity = AdditionalEntities[I];

        vec2i EntityRelPxP = (Vec2I(Entity->P) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
        DestRect.X = EntityRelPxP.X;
        DestRect.Y = EntityRelPxP.Y;
        RenderGlyph(GameState->FontAtlas, Entity->Glyph, ScreenImage, DestRect, Entity->BackgroundColor, Entity->ForegroundColor);
    }

    #if 0
    vec3i *Chunks[] = { &ChunkMin, &ChunkMax };

    for (u32 I = 0;
         I < ArrayCount(Chunks);
         ++I)
    {
        vec3i Min = GetLeftmostTilePFromChunkP(*Chunks[I], GameState->ChunkDim);
        vec3i Max = Min + GameState->ChunkDim;

        for (i32 Y = Min.Y;
             Y < Max.Y;
             ++Y)
        {
            for (i32 X = Min.X;
                 X < Max.X;
                 ++X)
            {
                vec2i EntityRelPxP = (Vec2I(X, Y) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
                DestRect.X = EntityRelPxP.X;
                DestRect.Y = EntityRelPxP.Y;
                RenderGlyph(GameState->FontAtlas, '+', ScreenImage, DestRect, Vec3(0,0,1), Vec3(0,1,0));
            }
        }
    }

    for (i32 X = TileMinX;
         X <= TileMaxX;
         ++X)
    {
        vec2i EntityRelPxP = (Vec2I(X, TileMinY) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
        DestRect.X = EntityRelPxP.X;
        DestRect.Y = EntityRelPxP.Y;
        RenderGlyph(GameState->FontAtlas, '#', ScreenImage, DestRect, Vec3(1), Vec3(0));

        EntityRelPxP = (Vec2I(X, TileMaxY) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
        DestRect.X = EntityRelPxP.X;
        DestRect.Y = EntityRelPxP.Y;
        RenderGlyph(GameState->FontAtlas, '#', ScreenImage, DestRect, Vec3(1), Vec3(0));

    }

    for (i32 Y = TileMinY;
         Y <= TileMaxY;
         ++Y)
    {
        vec2i EntityRelPxP = (Vec2I(TileMinX, Y) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
        DestRect.X = EntityRelPxP.X;
        DestRect.Y = EntityRelPxP.Y;
        RenderGlyph(GameState->FontAtlas, '#', ScreenImage, DestRect, Vec3(1), Vec3(0));

        EntityRelPxP = (Vec2I(TileMaxX, Y) - Vec2I(GameState->CameraCenterTile)) * GameState->TileDim + AllCameraOffsets;
        DestRect.X = EntityRelPxP.X;
        DestRect.Y = EntityRelPxP.Y;
        RenderGlyph(GameState->FontAtlas, '#', ScreenImage, DestRect, Vec3(1), Vec3(0));

    }
    #endif
}
