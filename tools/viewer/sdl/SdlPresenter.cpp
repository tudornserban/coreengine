#include "SdlPresenter.h"

#include "ViewerInput.h"

#include <SDL.h>

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
    bool middleDown = false;

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
                loop.ApplyViewerKeyAction(HandleViewerKeycode(event.key.keysym.sym));
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
                else if (event.button.button == SDL_BUTTON_MIDDLE)
                {
                    middleDown = true;
                    loop.OnMouseMiddleDown(screen);
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
                else if (event.button.button == SDL_BUTTON_MIDDLE)
                {
                    const Vector2 screen(
                        static_cast<float>(event.button.x),
                        static_cast<float>(event.button.y));
                    middleDown = false;
                    loop.OnMouseMiddleUp(screen);
                }
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                const Vector2 screen(
                    static_cast<float>(event.motion.x),
                    static_cast<float>(event.motion.y));
                if (middleDown)
                {
                    loop.OnMouseMiddleMotion(screen);
                }
                else
                {
                    loop.OnMouseMotion(screen);
                }
            }
            else if (event.type == SDL_MOUSEWHEEL)
            {
                const float mouseX = static_cast<float>(event.wheel.mouseX);
                const float mouseY = static_cast<float>(event.wheel.mouseY);
                loop.OnMouseWheel(mouseX, mouseY, static_cast<float>(event.wheel.y));
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
