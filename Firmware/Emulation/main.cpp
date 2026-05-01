// Tell SDL3 we are handling main() ourselves (no WinMain redirect needed
// when building a console-subsystem binary with MinGW).
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <cstring>

// CPS-1 native resolution (Street Fighter II reference)
static constexpr int CPS1_WIDTH  = 384;
static constexpr int CPS1_HEIGHT = 224;

// Initial window scale — keeps the 384x224 logical canvas legible on modern displays
static constexpr int WINDOW_SCALE = 3;

int main(int /*argc*/, char* /*argv*/[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "CPS-1 Emulator | 384x224",
        CPS1_WIDTH  * WINDOW_SCALE,
        CPS1_HEIGHT * WINDOW_SCALE,
        SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        SDL_Log("SDL_CreateWindow: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Scale the logical 384x224 canvas to the physical window using integer steps
    // so we never get sub-pixel smearing on the pixel art.
    SDL_SetRenderLogicalPresentation(renderer, CPS1_WIDTH, CPS1_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    // Streaming texture: the emulator will lock this every frame and write
    // RGB565 pixels resolved from the CPS-1 palette RAM.
    SDL_Texture* framebuffer = SDL_CreateTexture(
                                                    renderer,
                                                    SDL_PIXELFORMAT_RGB565,
                                                    SDL_TEXTUREACCESS_STREAMING,
                                                    CPS1_WIDTH, CPS1_HEIGHT
                                                );
    if (!framebuffer)
    {
        SDL_Log("SDL_CreateTexture: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // RGB565 0x0000 == black (same as CPS-1 palette index 0)
    {
        void* pixels = nullptr;
        int   pitch  = 0;
        SDL_LockTexture(framebuffer, nullptr, &pixels, &pitch);
        std::memset(pixels, 0, static_cast<std::size_t>(pitch) * CPS1_HEIGHT);
        SDL_UnlockTexture(framebuffer);
    }

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE)
                        running = false;
                    break;
                default:
                    break;
            }
        }

        // ---------------------------------------------------------------
        // Emulation tick goes here:
        //   1. Run 68k for one frame's worth of cycles
        //   2. Render CPS-1 layers + sprites into a staging buffer
        //   3. Lock framebuffer, blit staged RGB565 pixels, unlock
        // ---------------------------------------------------------------

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, framebuffer, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(framebuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
