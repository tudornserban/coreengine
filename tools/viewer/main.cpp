#include "Camera.h"
#include "DrawList.h"
#include "PngWriter.h"
#include "Raster.h"
#include "ViewerConfig.h"
#include "sdl/SdlPresenter.h"

#include <cstdlib>
#include <iostream>

namespace
{

DrawList BuildEmptyMapDrawList(const Camera& camera)
{
    DrawList list;
    list.clear = {20, 24, 28, 255};

    const Vector2 worldMin(0.0f, 0.0f);
    const Vector2 worldMax(800.0f, 600.0f);
    const Vector2 screenMin = camera.WorldToScreen(worldMin);
    const Vector2 screenMax = camera.WorldToScreen(worldMax);

    DrawRect fill;
    fill.min = screenMin;
    fill.max = screenMax;
    fill.filled = true;
    fill.color = {32, 36, 42, 255};
    list.rects.push_back(fill);

    DrawRect border;
    border.min = screenMin;
    border.max = screenMax;
    border.filled = false;
    border.color = {180, 180, 180, 255};
    list.rects.push_back(border);

    return list;
}

} // namespace

int main(int argc, char** argv)
{
    ViewerConfig config = ParseViewerConfig(argc, argv);
    if (config.parseError)
    {
        return 2;
    }

    Camera camera = FitMap(Vector2(800.0f, 600.0f), config.width, config.height, kFitMarginPx);
    DrawList list = BuildEmptyMapDrawList(camera);

    Framebuffer fb;
    fb.width = config.width;
    fb.height = config.height;
    Rasterize(list, fb);

    if (!config.screenshotPath.empty())
    {
        if (!WriteRgbaPng(config.screenshotPath, fb))
        {
            std::cerr << "Failed to write screenshot: " << config.screenshotPath << '\n';
            return 4;
        }
    }

    if (!config.noWindow)
    {
        if (!PresentFramebuffer(fb, config.frames, "CoreEngine Viewer"))
        {
            std::cerr << "SDL presentation failed\n";
            return 3;
        }
    }

    return 0;
}
