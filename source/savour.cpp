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

void
GameUpdateAndRender(game_input *GameInput, game_memory *GameMemory, platform_image *OffscreenBuffer, b32 *GameShouldQuit)
{
    game_state *GameState = (game_state *) GameMemory->Storage;

    b32 InitialPlayerPositionReset = false;
    if (!GameMemory->IsInitialized)
    {
        // TODO: Temporary
        srand((u32)time(NULL));

        GameState->FontAtlas.Image = GetImageFromPlatformImage(Platform_LoadBMP("resources/font.bmp"));
        GameState->FontAtlas.AtlasWidth = 16;
        GameState->FontAtlas.AtlasHeight = 16;
        GameState->FontAtlas.GlyphPxWidth = 48;
        GameState->FontAtlas.GlyphPxHeight = 72;

        GameState->ChunkDim = Vec3I(16,16,1);
        
        vec3i InitialPosition = Vec3I();
        vec3i InitialChunkPosition = GetChunkPFromTileP(InitialPosition, GameState->ChunkDim);

        vec3i InViewMinChunkX = InitialPosition;
        // InViewMinChunkX -= 
        

        #if 0
        for (i32 MapI = 0;
             MapI < GameState->MapWidth * GameState->MapHeight;
             ++MapI)
        {
            entity_node *HeadEntityNode = GameState->MapTable + MapI;
            
            i32 X = MapI % GameState->MapWidth;
            i32 Y = MapI / GameState->MapWidth;
            vec3i Position = Vec3I(X, Y, 0);

            f32 Intensity = PerlinSampleOctaves(X / 32.0f, Y / 32.0f, 1.8f, 0.5f, 6, 101);
            Intensity = PerlinNormalize(Intensity);

            if (Intensity <= 0.4f)
            {
                // NOTE: Water
                HeadEntityNode->E.Glyph = (((rand() % 2) ==  0) ? 247 : 126);
                HeadEntityNode->E.ForegroundColor = Vec3(0.3f, 0.3f, 0.8f);
                HeadEntityNode->E.BackgroundColor = Vec3(0.2f, 0.2f, 0.6f);
                HeadEntityNode->E.Position = Position;
                HeadEntityNode->E.IsBlocking = false;
                HeadEntityNode->E.IsOpaque = false;
                HeadEntityNode->IsPopulated = true;
            }
            else if (Intensity >= 0.6f)
            {
                // NOTE: Mountain
                HeadEntityNode->E.Glyph = 219;
                HeadEntityNode->E.ForegroundColor = Vec3(0.6f);
                HeadEntityNode->E.BackgroundColor = Vec3(0);
                HeadEntityNode->E.Position = Position;
                HeadEntityNode->E.IsBlocking = true;
                HeadEntityNode->E.IsOpaque = true;
                HeadEntityNode->IsPopulated = true;
            }
            else
            {
                // NOTE: Grass
                HeadEntityNode->E.Glyph = (((rand() % 2) ==  0) ? 176 : 177);
                HeadEntityNode->E.ForegroundColor = Vec3(0);
                HeadEntityNode->E.BackgroundColor = Vec3(0.3f, 0.6f, 0.4f);
                HeadEntityNode->E.Position = Position;
                HeadEntityNode->E.IsBlocking = false;
                HeadEntityNode->E.IsOpaque = false;
                HeadEntityNode->IsPopulated = true;
            }

            // if (X == 0 || X == (GameState->MapWidth - 1) ||
            //     Y == 0 || Y == (GameState->MapHeight - 1))
            // {
            //     entity_node *FreeEntityNode = GetFreeEntityNodeInMapTable(GameState);

            //     FreeEntityNode->E.Position = Position;
            //     FreeEntityNode->E.Glyph = '#';
            //     FreeEntityNode->E.ForegroundColor = Vec3(1);
            //     FreeEntityNode->E.BackgroundColor = Vec3(0);
            //     FreeEntityNode->E.IsBlocking = true;
            //     FreeEntityNode->E.IsOpaque = true;
            //     FreeEntityNode->IsPopulated = true;

            //     HeadEntityNode->Next = FreeEntityNode;
            // }
        }
        #endif
        
        GameState->CameraZoom = 1.0f;
        GameState->CameraInitialZoom = false;

        // NOTE: Create player entity
        {
            GameState->Player.P = InitialPosition;
            GameState->Player.Glyph = '@';
            GameState->Player.ForegroundColor = Vec3(0);
            GameState->Player.BackgroundColor = Vec3(0,0,1);
            GameState->Player.IsBlocking = true;
            GameState->Player.IsOpaque = false;

            InitialPlayerPositionReset = true;
        }

        {
            GameState->OtherEntity.P = InitialPosition + Vec3I(3, 3, 0);
            GameState->OtherEntity.Glyph = 'A';
            GameState->OtherEntity.ForegroundColor = Vec3(0);
            GameState->OtherEntity.BackgroundColor = Vec3(0,1,0);
            GameState->OtherEntity.IsBlocking = true;
            GameState->OtherEntity.IsOpaque = false;
        }

        GameMemory->IsInitialized = true;
    }

    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_ESCAPE))
    {
        *GameShouldQuit = true;
    }
    
    i32 ScreenGlyphWidth = 48;
    i32 ScreenGlyphHeight = 72;

    rect DestRect = {};
    DestRect.X = 0;
    DestRect.Y = 0;
    DestRect.Width = (i32) (ScreenGlyphWidth * GameState->CameraZoom);
    DestRect.Height = (i32) (ScreenGlyphHeight * GameState->CameraZoom);

    image ScreenImage = {};
    ScreenImage.Pixels = OffscreenBuffer->ImageData;
    ScreenImage.Width = OffscreenBuffer->Width;
    ScreenImage.Height = OffscreenBuffer->Height;

    b32 PlayerMoved = false;
    vec3i NewPlayerPosition = GameState->Player.P;
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_H))
    {
        NewPlayerPosition.X--;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_L))
    {
        NewPlayerPosition.X++;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_K))
    { 
        NewPlayerPosition.Y++;
        PlayerMoved = true;
    }
    if (Platform_KeyRepeat(GameInput, SDL_SCANCODE_J))
    {
        NewPlayerPosition.Y--;
        PlayerMoved = true;
    }
    GameState->Player.P = NewPlayerPosition;
    GameState->CameraCenterTile = GameState->Player.P;
    if (PlayerMoved)
    {
        printf("%d,%d,%d\n", GameState->Player.P.X, GameState->Player.P.Y, GameState->Player.P.Z);
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

    b32 RecalculateZoom = false;
    
    // NOTE: Move player entity, update camera offset
    if (PlayerMoved || InitialPlayerPositionReset)
    {
        entity_node *OldParent = GetEntityParentNode(GameState->MapTable, GameState->MapWidth, GameState->PlayerEntityNode);
        OldParent->Next = OldParent->Next->Next;

        entity_node *MapTopNodeInPosition = GetMapTopEntityNodeInPosition(GameState->MapTable, GameState->MapWidth, NewPlayerPosition);
        MapTopNodeInPosition->Next = GameState->PlayerEntityNode;
        GameState->PlayerEntityNode->Next = 0;
        
        GameState->PlayerEntityNode->E.Position.X = NewPlayerPosition.X;
        GameState->PlayerEntityNode->E.Position.Y = NewPlayerPosition.Y;

        RecalculateZoom = true;
        
        f32 PlayerCenterPxX = (f32) (GameState->PlayerEntityNode->E.Position.X * ScreenGlyphWidth) + (ScreenGlyphWidth / 2.0f);
        f32 PlayerCenterPxY = (f32) (GameState->PlayerEntityNode->E.Position.Y * ScreenGlyphHeight) + (ScreenGlyphHeight / 2.0f);

        f32 ZoomedHalfWidth = ScreenImage.Width / GameState->CameraZoom /  2.0f;
        f32 ZoomedHalfHeight = ScreenImage.Height / GameState->CameraZoom / 2.0f;
        GameState->CameraOffsetX = PlayerCenterPxX - ZoomedHalfWidth;
        GameState->CameraOffsetY = PlayerCenterPxY - ZoomedHalfHeight;
    }
    #endif

    // TODO: Maybe cull unseen entities on the level of MapI
    {
        {
            entity *Entity = &GameState->Player;

            vec2i GlyphDim = Vec2I(ScreenGlyphWidth, ScreenGlyphHeight);
            vec2i GlyphHalfDim = Vec2I(ScreenGlyphWidth / 2, ScreenGlyphHeight / 2);
            vec2i ScreenHalfDim = Vec2I(ScreenImage.Width / 2, ScreenImage.Height / 2);
            vec2i EntityRelPxP = (Vec2I(Entity->P) - Vec2I(GameState->CameraCenterTile)) * GlyphDim - GlyphHalfDim + ScreenHalfDim;
            DestRect.X = EntityRelPxP.X;
            DestRect.Y = EntityRelPxP.Y;
            RenderGlyph(GameState->FontAtlas, Entity->Glyph, ScreenImage, DestRect, Entity->BackgroundColor, Entity->ForegroundColor);

            #if 0
            DestRect.X = (i32) ((Entity->Position.X * ScreenGlyphWidth - (i32) GameState->CameraOffsetX) * GameState->CameraZoom);
            DestRect.Y = (i32) ((Entity->Position.Y * ScreenGlyphHeight - (i32) GameState->CameraOffsetY) * GameState->CameraZoom);
            #endif

        }
        {
            entity *Entity = &GameState->OtherEntity;

            vec2i GlyphDim = Vec2I(ScreenGlyphWidth, ScreenGlyphHeight);
            vec2i GlyphHalfDim = Vec2I(ScreenGlyphWidth / 2, ScreenGlyphHeight / 2);
            vec2i ScreenHalfDim = Vec2I(ScreenImage.Width / 2, ScreenImage.Height / 2);
            vec2i EntityRelPxP = (Vec2I(Entity->P) - Vec2I(GameState->CameraCenterTile)) * GlyphDim - GlyphHalfDim + ScreenHalfDim;
            DestRect.X = EntityRelPxP.X;
            DestRect.Y = EntityRelPxP.Y;
            RenderGlyph(GameState->FontAtlas, Entity->Glyph, ScreenImage, DestRect, Entity->BackgroundColor, Entity->ForegroundColor);
        }
    }

    if (Platform_MouseButtonIsDown(GameInput, MouseButton_Middle))
    {
        GameState->CameraOffsetX -= GameInput->MouseLogicalDeltaX / GameState->CameraZoom;
        GameState->CameraOffsetY -= GameInput->MouseLogicalDeltaY / GameState->CameraZoom;
    }

    f32 ZoomPerSecond = 5.0f;
    b32 ZoomChanged = false;
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_PAGEUP))
    {
        GameState->CameraZoom += ZoomPerSecond * GameInput->DeltaTime;
        ZoomChanged = true;
        GameState->CameraInitialZoom = false;
    }
    if (Platform_KeyIsDown(GameInput, SDL_SCANCODE_PAGEDOWN))
    {
        GameState->CameraZoom -= ZoomPerSecond * GameInput->DeltaTime;
        ZoomChanged = true;
        GameState->CameraInitialZoom = false;
    }

    if (PlayerMoved && GameState->CameraInitialZoom)
    {
        GameState->CameraZoom -= 1.0f;
        if (GameState->CameraZoom == 1.0f)
        {
            GameState->CameraInitialZoom = false;
        }
        ZoomChanged = true;
    }

    #if 0
    if (ZoomChanged)
    {
        f32 PlayerCenterPxX = (f32) (GameState->PlayerEntityNode->E.Position.X * ScreenGlyphWidth) + (ScreenGlyphWidth / 2.0f);
        f32 PlayerCenterPxY = (f32) (GameState->PlayerEntityNode->E.Position.Y * ScreenGlyphHeight) + (ScreenGlyphHeight / 2.0f);

        f32 ZoomedHalfWidth = ScreenImage.Width / GameState->CameraZoom /  2.0f;
        f32 ZoomedHalfHeight = ScreenImage.Height / GameState->CameraZoom / 2.0f;
        GameState->CameraOffsetX = PlayerCenterPxX - ZoomedHalfWidth;
        GameState->CameraOffsetY = PlayerCenterPxY - ZoomedHalfHeight;
    }
    #endif
}
