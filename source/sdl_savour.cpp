#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <sdl2/SDL.h>

#include "and_common.h"

int main(int argc, char **argv)
{
    i32 SDLInitResult = SDL_Init(SDL_INIT_VIDEO);
    Assert(SDLInitResult >= 0);

    SDL_Window *Window = SDL_CreateWindow("Savour",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1920, 1080,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    Assert(Window);

    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);

    u32 Width = 32;
    u32 Height = 24;
    SDL_Texture *ScreenTexture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, Width, Height);

    u32 *Pixels = (u32 *) calloc(1, Width * Height * 4);

    SDL_SetWindowMinimumSize(Window, Width, Height);

    SDL_RenderSetLogicalSize(Renderer, Width, Height);

    i32 FrameCount = 0;

    srand((u32)time(NULL));

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

        if (FrameCount > 100)
        {
            u32 *PixelCursor = Pixels;
            for (u32 PixelIndex = 0;
                 PixelIndex < Width * Height;
                 ++PixelIndex)
            {
                b32 IsGreen = rand() % 2;
                *PixelCursor++ = IsGreen ? 0x00FF00FF : 0xFF0000FF;
            }

            FrameCount = 0;
        }
        else
        {
            FrameCount++;
        }

        SDL_RenderClear(Renderer);
        SDL_UpdateTexture(ScreenTexture, NULL, Pixels, Width * 4);
        SDL_RenderCopy(Renderer, ScreenTexture, NULL, NULL);
        SDL_RenderPresent(Renderer);
    }

    SDL_DestroyWindow(Window);

    SDL_Quit();

    return 0;
}
