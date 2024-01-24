#ifndef SAVOUR_PLATFORM_H
#define SAVOUR_PLATFORM_H

#include <sdl2/SDL_scancode.h>

enum mouse_button_type
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Count,
};

struct game_input
{
    u8 CurrentKeyStates_[SDL_NUM_SCANCODES];
    u8 PreviousKeyStates_[SDL_NUM_SCANCODES];

    f32 KeyRepeatAccums_[SDL_NUM_SCANCODES];
    f32 KeyRepeatPeriod_;
    
    u8 CurrentMouseButtonStates_[MouseButton_Count];
    u8 PreviousMouseButtonStates_[MouseButton_Count];

    i32 MouseX;
    i32 MouseY;
    i32 MouseDeltaX;
    i32 MouseDeltaY;
    f32 MouseLogicalX;
    f32 MouseLogicalY;
    f32 MouseLogicalDeltaX;
    f32 MouseLogicalDeltaY;

    f32 DeltaTime;
};

struct game_memory
{
    b32 IsInitialized;

    size_t StorageSize;
    void *Storage;
};

struct platform_image
{
    i32 Width;
    i32 Height;
    void *ImageData;
    void *PointerToFree_;
};

void GameUpdateAndRender(game_input *GameInput, game_memory *GameMemory, platform_image *OffscreenBuffer, b32 *GameShouldQuit);

platform_image Platform_LoadBMP(const char *Path);
platform_image Platform_LoadImage(const char *Path);
void Platform_FreeImage(platform_image *PlatformImage);

inline b32
Platform_KeyIsDown(game_input *GameInput, u32 KeyScancode)
{
    b32 Result = GameInput->CurrentKeyStates_[KeyScancode];
    return Result;
}

inline b32
Platform_KeyJustPressed(game_input *GameInput, u32 KeyScancode)
{
    b32 Result = (GameInput->CurrentKeyStates_[KeyScancode] && !GameInput->PreviousKeyStates_[KeyScancode]);
    return Result;
}

inline b32
Platform_KeyJustReleased(game_input *GameInput, u32 KeyScancode)
{
    b32 Result = (!GameInput->CurrentKeyStates_[KeyScancode] && GameInput->PreviousKeyStates_[KeyScancode]);
    return Result;
}

inline b32
Platform_KeyRepeat(game_input *GameInput, u32 KeyScancode)
{
    // TODO: Quick and dirty, probably best to remember latest pressed key and run the accumulator just for it,
    // in the main UpdateInput routine, and then check it here
    if (GameInput->CurrentKeyStates_[KeyScancode])
    {
        if (GameInput->PreviousKeyStates_[KeyScancode])
        {
            // NOTE: Key was already pressed, run key repeat
            GameInput->KeyRepeatAccums_[KeyScancode] += GameInput->DeltaTime;
            if (GameInput->KeyRepeatAccums_[KeyScancode] >= GameInput->KeyRepeatPeriod_)
            {
                GameInput->KeyRepeatAccums_[KeyScancode] = 0.0f;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            // NOTE: Key newly pressed, reset key repeat accumulator, and return true
            GameInput->KeyRepeatAccums_[KeyScancode] =  0.0f;
            return true;
        }
    }
    else
    {
        return false;
    }
}

inline b32
Platform_MouseButtonIsDown(game_input *GameInput, mouse_button_type MouseButton)
{
    b32 Result = GameInput->CurrentMouseButtonStates_[MouseButton];
    return Result;
}

inline b32
Platform_MouseButtonJustPressed(game_input *GameInput, mouse_button_type MouseButton)
{
    b32 Result = (GameInput->CurrentMouseButtonStates_[MouseButton] && !GameInput->PreviousMouseButtonStates_[MouseButton]);
    return Result;
}

inline b32
Platform_MouseButtonJustReleased(game_input *GameInput, mouse_button_type MouseButton)
{
    b32 Result = (!GameInput->CurrentMouseButtonStates_[MouseButton] && GameInput->PreviousMouseButtonStates_[MouseButton]);
    return Result;
}

#endif
