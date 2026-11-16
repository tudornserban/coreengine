#include "SessionTypes.h"

#include <cctype>

namespace Session
{
namespace
{

std::string Lower(const std::string& text)
{
    std::string out = text;
    for (char& ch : out)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

}

const char* ToString(GameTitle game)
{
    switch (game)
    {
    case GameTitle::DynamicGo:
        return "DynamicGo";
    case GameTitle::AscendantExpanse:
        return "AscendantExpanse";
    case GameTitle::ZodiacWars:
        return "ZodiacWars";
    }
    return "DynamicGo";
}

const char* ToString(PlayMode mode)
{
    switch (mode)
    {
    case PlayMode::OfflineResolution:
        return "OfflineResolution";
    case PlayMode::OnlineSync:
        return "OnlineSync";
    }
    return "OfflineResolution";
}

bool TryParseGameTitle(const std::string& text, GameTitle& game)
{
    const std::string key = Lower(text);
    if (key == "dynamicgo")
    {
        game = GameTitle::DynamicGo;
        return true;
    }
    if (key == "ascendantexpanse")
    {
        game = GameTitle::AscendantExpanse;
        return true;
    }
    if (key == "zodiacwars")
    {
        game = GameTitle::ZodiacWars;
        return true;
    }
    return false;
}

bool TryParsePlayMode(const std::string& text, PlayMode& mode)
{
    const std::string key = Lower(text);
    if (key == "offlineresolution" || key == "offline")
    {
        mode = PlayMode::OfflineResolution;
        return true;
    }
    if (key == "onlinesync" || key == "online")
    {
        mode = PlayMode::OnlineSync;
        return true;
    }
    return false;
}

}
