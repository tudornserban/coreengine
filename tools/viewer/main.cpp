#include "Camera.h"
#include "DrawList.h"
#include "FrameLoop.h"
#include "InputScript.h"
#include "PngWriter.h"
#include "Scenario.h"
#include "ViewerConfig.h"
#include "sdl/SdlPresenter.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace
{

bool ReadFileToString(const std::string& path, std::string& contents)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    contents = buffer.str();
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    ViewerConfig config = ParseViewerConfig(argc, argv);
    if (config.parseError)
    {
        return 2;
    }

    FrameLoop loop(config.preset, kDefaultSeed, config.width, config.height);

    if (!config.scriptPath.empty())
    {
        std::string scriptText;
        if (!ReadFileToString(config.scriptPath, scriptText))
        {
            std::cerr << "Failed to read script: " << config.scriptPath << '\n';
            return 5;
        }
        std::string scriptError;
        const std::vector<ScriptCommand> commands = ParseScript(scriptText, scriptError);
        if (!scriptError.empty())
        {
            std::cerr << "Script parse error: " << scriptError << '\n';
            return 5;
        }
        if (!loop.ExecuteScript(commands, scriptError))
        {
            std::cerr << "Script error: " << scriptError << '\n';
            return 5;
        }
    }

    Framebuffer fb;
    if (!PresentSimulation(loop, config, fb))
    {
        std::cerr << "SDL presentation failed\n";
        return 3;
    }

    if (!config.screenshotPath.empty())
    {
        if (!WriteRgbaPng(config.screenshotPath, fb))
        {
            std::cerr << "Failed to write screenshot: " << config.screenshotPath << '\n';
            return 4;
        }
    }

    return 0;
}
