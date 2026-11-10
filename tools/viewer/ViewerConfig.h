#pragma once

#include <string>

struct ViewerConfig
{
    int frames = 0;
    std::string screenshotPath;
    bool noWindow = false;
    int width = 1280;
    int height = 720;
    std::string scriptPath;
    bool parseError = false;
};

ViewerConfig ParseViewerConfig(int argc, char** argv);
