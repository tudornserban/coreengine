#pragma once

#include <string>
#include <vector>

struct ScriptCommand
{
    enum class Kind
    {
        Frames,
        Pause,
        Resume,
        Step,
        Speed,
        Reset,
        Dest
    };

    Kind kind = Kind::Frames;
    int intArg = 0;
    float floatArg = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
};

std::vector<ScriptCommand> ParseScript(const std::string& text, std::string& error);
