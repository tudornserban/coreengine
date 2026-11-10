#include "core/gamestate/RTSGameState.h"

#include "Camera.h"
#include "DrawList.h"
#include "PngWriter.h"
#include "Raster.h"
#include "Scenario.h"
#include "SnapshotRenderer.h"
#include "ViewerConfig.h"
#include "sdl/SdlPresenter.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv)
{
    ViewerConfig config = ParseViewerConfig(argc, argv);
    if (config.parseError)
    {
        return 2;
    }

    Scene scene = BuildPreset(Preset::TwoUnits, kDefaultSeed);
    Camera camera =
        FitMap(scene.state->GetMapSize(), config.width, config.height, kFitMarginPx);
    DrawList list = BuildSnapshotDrawList(*scene.state, camera);

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
