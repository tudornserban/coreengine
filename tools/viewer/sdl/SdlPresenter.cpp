#include "SdlPresenter.h"

#include <SDL.h>

bool PresentFramebuffer(const Framebuffer& fb, int frames, const char* title)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return false;
    }

    SDL_Window* window = SDL_CreateWindow(
        title != nullptr ? title : "CoreEngine Viewer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        fb.width,
        fb.height,
        SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        SDL_Quit();
        return false;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == nullptr)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        fb.width,
        fb.height);
    if (texture == nullptr)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    const int targetFrames = frames > 0 ? frames : 0;
    int presented = 0;
    bool running = true;
    while (running && (targetFrames == 0 || presented < targetFrames))
    {
        if (SDL_UpdateTexture(texture, nullptr, fb.rgba.data(), fb.width * 4) != 0)
        {
            SDL_DestroyTexture(texture);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        ++presented;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        if (targetFrames == 0)
        {
            SDL_Delay(16);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}
