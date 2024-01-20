#ifndef SAVOUR_PLATFORM_H
#define SAVOUR_PLATFORM_H

struct game_input
{
};

struct game_memory
{
    b32 IsInitialized;

    size_t StorageSize;
    void *Storage;
};

struct platform_image
{
    u32 Width;
    u32 Height;
    void *ImageData;
    void *PointerToFree_;
};

void GameUpdateAndRender(game_input *GameInput, game_memory *GameMemory, platform_image *OffscreenBuffer, b32 *GameShouldQuit);

platform_image Platform_LoadBMP(const char *Path);
platform_image Platform_LoadImage(const char *Path);
void Platform_FreeImage(platform_image *PlatformImage);

#endif
