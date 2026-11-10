#include "SdlPresenter.h"

#include "Camera.h"
#include "Overlay.h"
#include "PlaybackControl.h"

#include <SDL.h>

namespace
{

ViewerKey KeyFromSdl(int sdlKey)
{
    switch (sdlKey)
    {
    case SDLK_SPACE:
        return ViewerKey::Space;
    case SDLK_1:
        return ViewerKey::Digit1;
    case SDLK_2:
        return ViewerKey::Digit2;
    case SDLK_3:
        return ViewerKey::Digit3;
    case SDLK_4:
        return ViewerKey::Digit4;
    case SDLK_PERIOD:
        return ViewerKey::Period;
    case SDLK_r:
        return ViewerKey::R;
    default:
        return ViewerKey::Unknown;
    }
}

void ApplyViewerKey(FrameLoop& loop, ViewerKey key)
{
    if (key == ViewerKey::Unknown)
    {
        return;
    }
    loop.ApplyPlaybackCommand(CommandForKey(key));
}

} // namespace

bool PresentSimulation(FrameLoop& loop, const ViewerConfig& config, Framebuffer& framebuffer)
{
    if (config.noWindow)
    {
        if (config.scriptPath.empty())
        {
            for (int i = 0; i < config.frames; ++i)
            {
                loop.AdvancePresentationFrame();
            }
        }
        loop.RasterizeCurrentView(framebuffer);
        return true;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return false;
    }

    SDL_Window* window = SDL_CreateWindow(
        "CoreEngine Viewer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config.width,
        config.height,
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
        config.width,
        config.height);
    if (texture == nullptr)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    const int targetFrames = config.frames > 0 ? config.frames : 0;
    int presented = 0;
    bool running = true;

    while (running && (targetFrames == 0 || presented < targetFrames))
    {
        loop.AdvancePresentationFrame();
        loop.RasterizeCurrentView(framebuffer);

        if (SDL_UpdateTexture(texture, nullptr, framebuffer.rgba.data(), config.width * 4) != 0)
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
            else if (event.type == SDL_KEYDOWN)
            {
                const int sym = event.key.keysym.sym;
                const int fKeyIndex = OverlayFKeyIndexFromKeycode(sym);
                if (fKeyIndex > 0)
                {
                    loop.ToggleOverlayFromFKey(fKeyIndex);
                }
                else
                {
                    ApplyViewerKey(loop, KeyFromSdl(sym));
                }
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                const Vector2 screen(
                    static_cast<float>(event.button.x),
                    static_cast<float>(event.button.y));
                const bool shiftHeld = (SDL_GetModState() & KMOD_SHIFT) != 0;
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    loop.OnMouseLeftDown(screen, shiftHeld);
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    loop.OnMouseRightDown(screen, shiftHeld);
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    const Vector2 screen(
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y));
                    const bool shiftHeld = (SDL_GetModState() & KMOD_SHIFT) != 0;
                    loop.OnMouseLeftUp(screen, shiftHeld);
                }
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                const Vector2 screen(
                    static_cast<float>(event.motion.x),
                    static_cast<float>(event.motion.y));
                loop.OnMouseMotion(screen);
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
