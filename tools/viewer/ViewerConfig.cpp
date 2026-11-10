#include "ViewerConfig.h"

#include <cstdlib>
#include <cstring>
#include <string>

ViewerConfig ParseViewerConfig(int argc, char** argv)
{
    ViewerConfig config;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--frames") == 0)
        {
            if (i + 1 >= argc)
            {
                config.parseError = true;
                return config;
            }
            char* end = nullptr;
            long value = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value < 0)
            {
                config.parseError = true;
                return config;
            }
            config.frames = static_cast<int>(value);
        }
        else if (std::strcmp(arg, "--screenshot") == 0)
        {
            if (i + 1 >= argc)
            {
                config.parseError = true;
                return config;
            }
            config.screenshotPath = argv[++i];
        }
        else if (std::strcmp(arg, "--no-window") == 0)
        {
            config.noWindow = true;
        }
        else if (std::strcmp(arg, "--width") == 0)
        {
            if (i + 1 >= argc)
            {
                config.parseError = true;
                return config;
            }
            char* end = nullptr;
            long value = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value <= 0)
            {
                config.parseError = true;
                return config;
            }
            config.width = static_cast<int>(value);
        }
        else if (std::strcmp(arg, "--height") == 0)
        {
            if (i + 1 >= argc)
            {
                config.parseError = true;
                return config;
            }
            char* end = nullptr;
            long value = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || value <= 0)
            {
                config.parseError = true;
                return config;
            }
            config.height = static_cast<int>(value);
        }
        else
        {
            config.parseError = true;
            return config;
        }
    }

    if (config.noWindow && config.frames == 0)
    {
        config.frames = 1;
    }

    return config;
}
