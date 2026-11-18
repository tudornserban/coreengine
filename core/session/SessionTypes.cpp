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
    case GameTitle::VenusWars:
        return "VenusWars";
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
    if (key == "venuswars")
    {
        game = GameTitle::VenusWars;
        return true;
    }
    return false;
}

SetupReject ValidateMatchSetup(const MatchSetup& setup)
{
    if (setup.schemaVersion != kMatchSetupSchemaVersion)
    {
        return SetupReject::UnsupportedVersion;
    }
    if (setup.payload.size() > kMaxSetupPayloadBytes || setup.contentReference.size() > kMaxSetupReferenceLength)
    {
        return SetupReject::Oversized;
    }
    for (unsigned char ch : setup.contentReference)
    {
        const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' || ch == '/';
        if (!ok)
        {
            return SetupReject::InvalidReference;
        }
    }
    return SetupReject::Ok;
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
