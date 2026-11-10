#include "InputScript.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace
{

std::string Trim(const std::string& line)
{
    size_t start = 0;
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
    {
        ++start;
    }
    size_t end = line.size();
    while (end > start && std::isspace(static_cast<unsigned char>(line[end - 1])))
    {
        --end;
    }
    return line.substr(start, end - start);
}

bool ParseInt(const std::string& token, int& value)
{
    char* end = nullptr;
    long v = std::strtol(token.c_str(), &end, 10);
    if (end == token.c_str() || *end != '\0')
    {
        return false;
    }
    value = static_cast<int>(v);
    return true;
}

bool ParseFloat(const std::string& token, float& value)
{
    char* end = nullptr;
    float v = std::strtof(token.c_str(), &end);
    if (end == token.c_str() || *end != '\0')
    {
        return false;
    }
    value = v;
    return true;
}

bool ParseLine(const std::string& line, ScriptCommand& cmd, std::string& error)
{
    if (line.empty() || line[0] == '#')
    {
        return false;
    }
    if (line.find(',') != std::string::npos)
    {
        error = "commas are not allowed in script lines";
        return false;
    }

    std::istringstream iss(line);
    std::string verb;
    iss >> verb;
    if (verb == "frames")
    {
        std::string countToken;
        if (!(iss >> countToken))
        {
            error = "frames requires a count";
            return true;
        }
        cmd.kind = ScriptCommand::Kind::Frames;
        if (!ParseInt(countToken, cmd.intArg) || cmd.intArg < 0)
        {
            error = "invalid frames count";
            return true;
        }
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after frames";
            return true;
        }
        return true;
    }
    if (verb == "pause")
    {
        cmd.kind = ScriptCommand::Kind::Pause;
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after pause";
            return true;
        }
        return true;
    }
    if (verb == "resume")
    {
        cmd.kind = ScriptCommand::Kind::Resume;
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after resume";
            return true;
        }
        return true;
    }
    if (verb == "step")
    {
        cmd.kind = ScriptCommand::Kind::Step;
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after step";
            return true;
        }
        return true;
    }
    if (verb == "speed")
    {
        std::string speedToken;
        if (!(iss >> speedToken))
        {
            error = "speed requires a multiplier";
            return true;
        }
        float speed = 0.0f;
        if (!ParseFloat(speedToken, speed))
        {
            error = "invalid speed";
            return true;
        }
        if (speed != 0.5f && speed != 1.0f && speed != 2.0f && speed != 4.0f)
        {
            error = "speed must be 0.5, 1, 2, or 4";
            return true;
        }
        cmd.kind = ScriptCommand::Kind::Speed;
        cmd.floatArg = speed;
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after speed";
            return true;
        }
        return true;
    }
    if (verb == "reset")
    {
        cmd.kind = ScriptCommand::Kind::Reset;
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after reset";
            return true;
        }
        return true;
    }
    if (verb == "dest")
    {
        std::string indexToken;
        std::string xToken;
        std::string yToken;
        if (!(iss >> indexToken >> xToken >> yToken))
        {
            error = "dest requires index x y";
            return true;
        }
        cmd.kind = ScriptCommand::Kind::Dest;
        if (!ParseInt(indexToken, cmd.intArg))
        {
            error = "invalid dest index";
            return true;
        }
        if (!ParseFloat(xToken, cmd.x) || !ParseFloat(yToken, cmd.y))
        {
            error = "invalid dest coordinates";
            return true;
        }
        std::string extra;
        if (iss >> extra)
        {
            error = "unexpected token after dest";
            return true;
        }
        return true;
    }
    if (verb == "click" || verb == "rclick" || verb == "box")
    {
        error = "mouse script commands are not supported until V1.4";
        return true;
    }

    error = "unknown script command: " + verb;
    return true;
}

} // namespace

std::vector<ScriptCommand> ParseScript(const std::string& text, std::string& error)
{
    error.clear();
    std::vector<ScriptCommand> commands;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
    {
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
        {
            continue;
        }
        ScriptCommand cmd;
        if (ParseLine(trimmed, cmd, error))
        {
            if (!error.empty())
            {
                commands.clear();
                return commands;
            }
            commands.push_back(cmd);
        }
    }
    return commands;
}
