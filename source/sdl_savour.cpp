#include <cstdio>
#include <cstdlib>

#include <sdl2/SDL.h>

#include "and_common.h"
#include "and_math.h"
#include "and_linmath.h"
#include "and_random.h"

#include "savour_platform.h"

internal void UpdateInput(SDL_Renderer *Render, game_input *GameInput);

int main(int argc, char **argv)
{
    i32 SDLInitResult = SDL_Init(SDL_INIT_VIDEO);
    Assert(SDLInitResult >= 0);

    SDL_Window *Window = SDL_CreateWindow("Savour",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1920, 1080,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    Assert(Window);

    i32 ClientWidth;
    i32 ClientHeight;
    SDL_GetWindowSize(Window, &ClientWidth, &ClientHeight);

    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);
    Assert(Renderer);
    
    platform_image OffscreenBuffer = {};
    OffscreenBuffer.Width = ClientWidth;
    OffscreenBuffer.Height = ClientHeight;
    u32 BytesPerPixel = 4;
    OffscreenBuffer.ImageData = calloc(1, OffscreenBuffer.Width * OffscreenBuffer.Height * BytesPerPixel);
    Assert(OffscreenBuffer.ImageData);
    
    SDL_SetWindowMinimumSize(Window, OffscreenBuffer.Width, OffscreenBuffer.Height);
    SDL_RenderSetLogicalSize(Renderer, OffscreenBuffer.Width, OffscreenBuffer.Height);
    SDL_Texture *OffscreenTexture = SDL_CreateTexture(Renderer,
                                                      SDL_PIXELFORMAT_RGBA8888,
                                                      SDL_TEXTUREACCESS_STREAMING,
                                                      OffscreenBuffer.Width,
                                                      OffscreenBuffer.Height);
    Assert(OffscreenTexture);

    game_input *GameInput = (game_input *) calloc(1, sizeof(game_input));
     // TODO: Maybe these 2 should be set and stored elsewhere
    GameInput->KeyRepeatDelay_ = 0.2f;
    GameInput->KeyRepeatPeriod_ = 0.09f;
    game_memory GameMemory = {};
    GameMemory.StorageSize = Megabytes(64);
    GameMemory.Storage = calloc(1, GameMemory.StorageSize);
    Assert(GameMemory.Storage);

    u64 PerfCounterFrequency = SDL_GetPerformanceFrequency();
    u64 LastCounter = SDL_GetPerformanceCounter();
    f64 PrevFrameDeltaTimeSec = 0.0f;
    f64 FPS = 0.0f;

    u32 *TestPerlinPixels = (u32 *) calloc(1, 1024 * 1024 * sizeof(u32));

    perlin_state *PerlinState = (perlin_state *) calloc(1, sizeof(perlin_state));
    SeedPerlin(PerlinState, 101);
    
    u32 *CopyPixel = TestPerlinPixels;
    u32 PixelCount = 0;
    for (u32 Y = 0;
         Y < 1024;
         ++Y)
    {
        for (u32 X = 0;
             X < 1024;
             ++X)
        {
            f32 Intensity = PerlinSample(PerlinState, X / 400.0f, Y / 400.0f) * 0.5f + 0.5f;
            if (Intensity < 0.0f)
            {
                printf("%d: %f\n", PixelCount, Intensity);
            }
            u8 IntensityI = (u8) (255 * Intensity);
            u32 Color = (IntensityI << 24 |
                         IntensityI << 16 |
                         IntensityI << 8 |
                         255);
            // printf("%x\n", Color);
            *CopyPixel++ = Color;
            PixelCount++;
        }
    }
    
    SDL_Surface *TestPerlinSurface = SDL_CreateRGBSurfaceFrom((void *) TestPerlinPixels,
                                                              1024,
                                                              1024,
                                                              32, // depth in bits
                                                              1024 * 4, // pitch in bytes
                                                              0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    
    i32 Result = SDL_SaveBMP(TestPerlinSurface, "temp/testPerlin.bmp");
    printf("%s\n", SDL_GetError());
    // Assert(Result);
    
    b32 ShouldQuit = false;
    while (!ShouldQuit)
    {
        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            switch (Event.type)
            {
                case SDL_QUIT:
                {
                    ShouldQuit = true;
                } break;
            }
        }

        //
        // NOTE: Update input
        //
        UpdateInput(Renderer, GameInput);
        GameInput->DeltaTime = (f32) PrevFrameDeltaTimeSec;

        //
        // NOTE: Run game
        //
        GameUpdateAndRender(GameInput, &GameMemory, &OffscreenBuffer, &ShouldQuit);

        //
        // NOTE: Flip buffer
        //
        SDL_RenderClear(Renderer);
        SDL_UpdateTexture(OffscreenTexture, NULL, OffscreenBuffer.ImageData, OffscreenBuffer.Width * BytesPerPixel);
        // TODO INVESTIGATE: is there double double buffer? We're copying, and then "presenting"
        SDL_RenderCopy(Renderer, OffscreenTexture, NULL, NULL);
        SDL_RenderPresent(Renderer);
        // NOTE: Clear offscreen buffer
        // TODO: Move to game
        u32 *Pixel = (u32 *) OffscreenBuffer.ImageData;
        for (i32 PixelIndex = 0;
             PixelIndex < OffscreenBuffer.Width * OffscreenBuffer.Height;
             ++PixelIndex)
        {
            *Pixel++ = 0xFF0000FF;
        }

        //
        // NOTE: Performance counter
        //
        u64 CurrentCounter = SDL_GetPerformanceCounter();
        u64 CounterElapsed = CurrentCounter - LastCounter;
        LastCounter = CurrentCounter;
        PrevFrameDeltaTimeSec = (f64) CounterElapsed / (f64) PerfCounterFrequency;
        FPS = 1.0 / PrevFrameDeltaTimeSec;

        char Title[256];
        sprintf_s(Title, "Savour [%0.3fFPS|%0.3fms]", FPS, PrevFrameDeltaTimeSec * 1000.0);
        SDL_SetWindowTitle(Window, Title);
    }

    SDL_DestroyWindow(Window);

    SDL_Quit();

    return 0;
}

internal void
UpdateInput(SDL_Renderer *Renderer, game_input *GameInput)
{
    const u8 *SDLKeyboardState = SDL_GetKeyboardState(0);
    for (u32 ScancodeIndex = 0;
         ScancodeIndex < SDL_NUM_SCANCODES;
         ++ScancodeIndex)
    {
        GameInput->PreviousKeyStates_[ScancodeIndex] = GameInput->CurrentKeyStates_[ScancodeIndex];
        GameInput->CurrentKeyStates_[ScancodeIndex] = (SDLKeyboardState[ScancodeIndex] != 0);
    }

    u32 SDLMouseButtonState = SDL_GetMouseState(&GameInput->MouseX, &GameInput->MouseY);
    for (u32 MouseButtonIndex = 0;
         MouseButtonIndex < MouseButton_Count;
         ++MouseButtonIndex)
    {
        GameInput->PreviousMouseButtonStates_[MouseButtonIndex] = GameInput->CurrentMouseButtonStates_[MouseButtonIndex];
        GameInput->CurrentMouseButtonStates_[MouseButtonIndex] = (SDLMouseButtonState & SDL_BUTTON(MouseButtonIndex + 1));
    }

    SDL_GetRelativeMouseState(&GameInput->MouseDeltaX, &GameInput->MouseDeltaY);

    // HACK
    f32 X, Y;
    SDL_RenderWindowToLogical(Renderer, 
                              GameInput->MouseX + GameInput->MouseDeltaX, GameInput->MouseY + GameInput->MouseDeltaY,
                              &X, &Y);

    SDL_RenderWindowToLogical(Renderer, 
                              GameInput->MouseX, GameInput->MouseY, 
                              &GameInput->MouseLogicalX, &GameInput->MouseLogicalY);

    GameInput->MouseLogicalDeltaX = X - GameInput->MouseLogicalX;
    GameInput->MouseLogicalDeltaY = Y - GameInput->MouseLogicalY;
}

platform_image
Platform_LoadBMP(const char *Path)
{
    platform_image Result = {};
    
    SDL_Surface *OriginalSurface = SDL_LoadBMP(Path);
    SDL_Surface *RGBASurface = SDL_ConvertSurfaceFormat(OriginalSurface, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(OriginalSurface);

    Result.Width = RGBASurface->w;
    Result.Height = RGBASurface->h;
    Result.ImageData = RGBASurface->pixels;
    Result.PointerToFree_ = (void *) RGBASurface;
    
    return Result;
}

void
Platform_FreeImage(platform_image *PlatformImage)
{
    SDL_FreeSurface((SDL_Surface *) PlatformImage->PointerToFree_);
    PlatformImage->ImageData = 0;
    PlatformImage->PointerToFree_ = 0;
}
