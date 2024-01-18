#include <cstdio>
#include <cstdlib>

#include <sdl2/SDL.h>

#include "and_common.h"

global_variable b32 GlobalRunning;

struct sdl_offscreen_buffer
{
    SDL_Texture *Texture;
    void *Pixels;
    i32 Width;
    i32 Height;
    i32 BytesPerPixel = 4;
};

global_variable sdl_offscreen_buffer GlobalBackbuffer;

void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height)
{
    if (GlobalBackbuffer.Texture)
    {
        SDL_DestroyTexture(GlobalBackbuffer.Texture);
        GlobalBackbuffer.Texture = 0;
    }
    if (GlobalBackbuffer.Pixels)
    {
        free(GlobalBackbuffer.Pixels);
    }
    
    GlobalBackbuffer.Texture = SDL_CreateTexture(Renderer,
                                      SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      Width, Height);

    GlobalBackbuffer.Pixels = calloc(1, Width * Height * GlobalBackbuffer.BytesPerPixel);

    GlobalBackbuffer.Width = Width;
    GlobalBackbuffer.Height = Height;
}

void SDLUpdateWindow(SDL_Renderer *Renderer)
{
    Assert(SDL_UpdateTexture(GlobalBackbuffer.Texture,
                             0,
                             GlobalBackbuffer.Pixels,
                             GlobalBackbuffer.Width * GlobalBackbuffer.BytesPerPixel) == 0);

    SDL_RenderCopy(Renderer,
                   GlobalBackbuffer.Texture,
                   0,
                   0);

    SDL_RenderPresent(Renderer);
}

void RenderWeirdGradient(u32 BlueOffset, u32 GreenOffset)
{
    i32 Width = GlobalBackbuffer.Width;
    i32 Height = GlobalBackbuffer.Height;

    i32 Pitch = Width * GlobalBackbuffer.BytesPerPixel;

    u8 *Row = (u8 *) GlobalBackbuffer.Pixels;
    for (i32 Y = 0;
         Y < Height;
         ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for (i32 X = 0;
             X < Width;
             ++X)
        {
            u8 Blue = (u8) X + (u8) BlueOffset;
            u8 Green = (u8) Y + (u8) GreenOffset;
            
            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Pitch;
    }
         
}

void HandleEvent(SDL_Event *Event)
{
    switch(Event->type)
    {
        case SDL_QUIT:
        {
            printf("SDL_QUIT\n");
            GlobalRunning = false;
        } break;

        case SDL_WINDOWEVENT:
        {
            switch (Event->window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    printf("SDL_WINDOWEVENT_RESIZED (%d, %d) \n", Event->window.data1, Event->window.data2);
                } break;

                case SDL_WINDOWEVENT_EXPOSED:
                {
                    SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
                    SDL_Renderer *Renderer = SDL_GetRenderer(Window);

                    int Width, Height;
                    SDL_GetWindowSize(Window, &Width, &Height);

                    SDLResizeTexture(Renderer, Width, Height);

                    SDLUpdateWindow(Renderer);

                    #if 0
                    local_persist bool IsWhite = true;
                    if (IsWhite)
                    {
                        SDL_SetRenderDrawColor(Renderer, 255, 255, 255, 255);
                        IsWhite = false;
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(Renderer, 0, 0, 0, 255);
                        IsWhite = true;
                    }

                    SDL_RenderClear(Renderer);
                    SDL_RenderPresent(Renderer);
                    #endif
                } break;
            }
        } break;
    }
}

int main(int argc, char **argv)
{
    i32 SDLInitResult = SDL_Init(SDL_INIT_VIDEO);
    Assert(SDLInitResult >= 0);

    SDL_Window *Window = SDL_CreateWindow("Savour",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          1920, 1080,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    Assert(Window);

    #if 0
    SDL_Surface *WindowSurface = SDL_GetWindowSurface(Window);

    SDL_FillRect(WindowSurface, NULL, SDL_MapRGB(WindowSurface->format, 0xFF, 0x00, 0x00));

    SDL_UpdateWindowSurface(Window);
    #endif

    SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, SDL_RENDERER_PRESENTVSYNC);

    Assert (Renderer);

    int Width, Height;
    SDL_GetWindowSize(Window, &Width, &Height);
    
    SDLResizeTexture(Renderer, Width, Height);
    
    GlobalRunning = true;

    u32 XOffset = 0;
    u32 YOffset = 0;
    
    while (GlobalRunning)
    {
        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            HandleEvent(&Event);
        }

        RenderWeirdGradient(XOffset, YOffset);

        SDLUpdateWindow(Renderer);

        XOffset++;
        YOffset += 2;
    }

    SDL_DestroyWindow(Window);

    SDL_Quit();

    return 0;
}
