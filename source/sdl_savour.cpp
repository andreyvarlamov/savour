#include <cstdio>
#include <cstdlib>

#include <sdl2/SDL.h>

#include "and_common.h"

int main(int argc, char **argv)
{
    i32 SDLInitResult = SDL_Init(SDL_INIT_VIDEO);
    Assert(SDLInitResult >= 0);

    SDL_Window *Window = SDL_CreateWindow("Savour",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1920, 1080,
                                          SDL_WINDOW_SHOWN);
    Assert(Window);

    SDL_Surface *WindowSurface = SDL_GetWindowSurface(Window);

    SDL_FillRect(WindowSurface, NULL, SDL_MapRGB(WindowSurface->format, 0x00, 0x00, 0x00));

    SDL_UpdateWindowSurface(Window);

    u32 *Pixels = (u32 *) WindowSurface->pixels;
    u32 PixelCount = WindowSurface->w * WindowSurface->h;

    i32 FrameCount = 0;
    b32 IsGreen = false;

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
            u32 PixelColor = IsGreen ? 0x00FF00FF : 0xFF0000FF;
            u32 *PixelCursor = Pixels;
            for (u32 PixelIndex = 0;
                 PixelIndex < PixelCount;
                 ++PixelIndex)
            {
                *PixelCursor++ = PixelColor;
            }

            IsGreen = !IsGreen;
            FrameCount = 0;
        }
        else
        {
            FrameCount++;
        }
            
        SDL_UpdateWindowSurface(Window);
    }

    SDL_DestroyWindow(Window);

    SDL_Quit();

    return 0;
}
