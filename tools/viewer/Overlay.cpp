#include "Overlay.h"

#include "core/ai/BehaviorContext.h"
#include "core/interfaces/IGameUnit.h"
#include "core/interfaces/IBehavior.h"
#include "core/interfaces/IUnitBehaviorHost.h"
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/map/MapComponents/MapObstacleComponent.h"
#include "core/map/MapEntity.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{

constexpr float kMarkerWorldRadius = 6.0f;

Rgba ColorForInfluenceType(InfluenceType type)
{
    switch (type)
    {
    case InfluenceType::EDGE:
        return {0, 220, 220, 255};
    case InfluenceType::ATTRACTOR:
        return {80, 220, 80, 255};
    case InfluenceType::REPULSOR:
        return {220, 60, 60, 255};
    default:
        return {200, 200, 200, 255};
    }
}

void PushHollowCircle(DrawList& list, Vector2 center, float radiusPx, Rgba outline)
{
    DrawCircle circle;
    circle.center = center;
    circle.radius = radiusPx;
    circle.fill = kViewerHollowCircleFill;
    circle.outline = outline;
    list.circles.push_back(circle);
}

void PushFilledCircle(DrawList& list, Vector2 center, float radiusPx, Rgba color)
{
    DrawCircle circle;
    circle.center = center;
    circle.radius = radiusPx;
    circle.fill = color;
    circle.outline = color;
    list.circles.push_back(circle);
}

void PushCollisionCross(DrawList& list, Vector2 screenCenter, Rgba color)
{
    const float half = 3.0f;
    DrawLine h;
    h.a = Vector2(screenCenter.x - half, screenCenter.y);
    h.b = Vector2(screenCenter.x + half, screenCenter.y);
    h.color = color;
    list.lines.push_back(h);
    DrawLine v;
    v.a = Vector2(screenCenter.x, screenCenter.y - half);
    v.b = Vector2(screenCenter.x, screenCenter.y + half);
    v.color = color;
    list.lines.push_back(v);
}

// 5x7 bitmap font: one byte per row, top bit = left pixel.
constexpr std::uint8_t kFontSpace[7] = {0, 0, 0, 0, 0, 0, 0};

constexpr std::uint8_t kFont0[7] = {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e};
constexpr std::uint8_t kFont1[7] = {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e};
constexpr std::uint8_t kFont2[7] = {0x0e, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1f};
constexpr std::uint8_t kFont3[7] = {0x0e, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0e};
constexpr std::uint8_t kFont4[7] = {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02};
constexpr std::uint8_t kFont5[7] = {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e};
constexpr std::uint8_t kFont6[7] = {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e};
constexpr std::uint8_t kFont7[7] = {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
constexpr std::uint8_t kFont8[7] = {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e};
constexpr std::uint8_t kFont9[7] = {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c};

constexpr std::uint8_t kFontP[7] = {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10};
constexpr std::uint8_t kFontA[7] = {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
constexpr std::uint8_t kFontU[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
constexpr std::uint8_t kFontS[7] = {0x0e, 0x11, 0x10, 0x0e, 0x01, 0x11, 0x0e};
constexpr std::uint8_t kFontE[7] = {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f};
constexpr std::uint8_t kFontO[7] = {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
constexpr std::uint8_t kFontV[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04};
constexpr std::uint8_t kFontR[7] = {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11};
constexpr std::uint8_t kFontL[7] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f};
constexpr std::uint8_t kFontY[7] = {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04};
constexpr std::uint8_t kFontI[7] = {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e};
constexpr std::uint8_t kFontN[7] = {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
constexpr std::uint8_t kFontC[7] = {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e};
constexpr std::uint8_t kFontB[7] = {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e};
constexpr std::uint8_t kFontH[7] = {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
constexpr std::uint8_t kFontX[7] = {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11};
constexpr std::uint8_t kFontEq[7] = {0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00};
constexpr std::uint8_t kFontDot[7] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c};

const std::uint8_t* GlyphForChar(char ch)
{
    switch (ch)
    {
    case ' ':
        return kFontSpace;
    case '0':
        return kFont0;
    case '1':
        return kFont1;
    case '2':
        return kFont2;
    case '3':
        return kFont3;
    case '4':
        return kFont4;
    case '5':
        return kFont5;
    case '6':
        return kFont6;
    case '7':
        return kFont7;
    case '8':
        return kFont8;
    case '9':
        return kFont9;
    case 'p':
        return kFontP;
    case 'a':
        return kFontA;
    case 'u':
        return kFontU;
    case 's':
        return kFontS;
    case 'e':
        return kFontE;
    case 'o':
        return kFontO;
    case 'v':
        return kFontV;
    case 'r':
        return kFontR;
    case 'l':
        return kFontL;
    case 'y':
        return kFontY;
    case 'i':
        return kFontI;
    case 'n':
        return kFontN;
    case 'c':
        return kFontC;
    case 'b':
        return kFontB;
    case 'h':
        return kFontH;
    case 'x':
        return kFontX;
    case '=':
        return kFontEq;
    case '.':
        return kFontDot;
    default:
        return kFontSpace;
    }
}

void DrawGlyph(DrawList& list, char ch, float originX, float originY, Rgba color)
{
    const std::uint8_t* glyph = GlyphForChar(ch);
    for (int row = 0; row < 7; ++row)
    {
        const std::uint8_t bits = glyph[row];
        for (int col = 0; col < 5; ++col)
        {
            if ((bits >> (4 - col)) & 1)
            {
                DrawRect pixel;
                pixel.min = Vector2(originX + static_cast<float>(col), originY + static_cast<float>(row));
                pixel.max = Vector2(pixel.min.x, pixel.min.y);
                pixel.filled = true;
                pixel.color = color;
                list.rects.push_back(pixel);
            }
        }
    }
}

void DrawText(DrawList& list, const std::string& text, float x, float y, Rgba color)
{
    float cursor = x;
    for (char ch : text)
    {
        DrawGlyph(list, ch, cursor, y, color);
        cursor += 6.0f;
    }
}

std::string OverlayBitsString(const OverlayFlags& flags)
{
    std::string bits;
    bits.push_back(flags.collisions ? '1' : '0');
    bits.push_back(flags.perception ? '1' : '0');
    bits.push_back(flags.influence ? '1' : '0');
    bits.push_back(flags.obstacles ? '1' : '0');
    bits.push_back(flags.behaviorItems ? '1' : '0');
    bits.push_back(flags.explosions ? '1' : '0');
    return bits;
}

std::string FormatSpeed(float multiplier)
{
    if (multiplier == 0.5f)
    {
        return "0.5";
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(multiplier));
    return buf;
}

} // namespace

void CollisionRecorder::OnCollision(const CollisionEvent& event)
{
    events_.push_back(event);
    eventTimes_.push_back(referenceGameTime_);
}

void CollisionRecorder::ExpireOlderThan(float gameTime, float keepSeconds)
{
    const float cutoff = gameTime - keepSeconds;
    std::size_t write = 0;
    for (std::size_t i = 0; i < events_.size(); ++i)
    {
        if (eventTimes_[i] >= cutoff)
        {
            events_[write] = events_[i];
            eventTimes_[write] = eventTimes_[i];
            ++write;
        }
    }
    events_.resize(write);
    eventTimes_.resize(write);
}

void CollisionRecorder::Clear()
{
    events_.clear();
    eventTimes_.clear();
}

void NoteExplosions(const RTSGameState& state, std::vector<ExplosionEcho>& echoes, float /*dt*/)
{
    for (const Explosion& explosion : state.GetExplosions())
    {
        ExplosionEcho echo;
        echo.position = explosion.GetPosition();
        echo.radius = explosion.GetRadius();
        echo.secondsLeft = kExplosionEchoLifetimeSeconds;
        echoes.push_back(echo);
    }
}

void TickExplosionEchoes(std::vector<ExplosionEcho>& echoes, float dt)
{
    for (ExplosionEcho& echo : echoes)
    {
        echo.secondsLeft -= dt;
    }
    echoes.erase(
        std::remove_if(
            echoes.begin(),
            echoes.end(),
            [](const ExplosionEcho& e) { return e.secondsLeft <= 0.0f; }),
        echoes.end());
}

void AppendOverlays(
    DrawList& list,
    RTSGameState& state,
    const Camera& camera,
    const OverlayFlags& flags,
    const CollisionRecorder& collisions,
    const std::vector<ExplosionEcho>& echoes)
{
    if (flags.collisions)
    {
        for (const CollisionEvent& event : collisions.Events())
        {
            const Vector2 contactScreen = camera.WorldToScreen(event.contactPoint);
            PushCollisionCross(list, contactScreen, kViewerCollisionOverlayColor);
            const Vector2 normalEnd = event.contactPoint + event.normal * 20.0f;
            DrawLine normalLine;
            normalLine.a = contactScreen;
            normalLine.b = camera.WorldToScreen(normalEnd);
            normalLine.color = kViewerCollisionOverlayColor;
            list.lines.push_back(normalLine);
        }
    }

    if (flags.perception)
    {
        const float ringRadius = state.GetPerceptionRadius() * camera.pixelsPerUnit;
        for (IGameUnit* unit : state.GetAllUnits())
        {
            if (unit == nullptr)
            {
                continue;
            }
            PushHollowCircle(
                list,
                camera.WorldToScreen(unit->GetPosition()),
                ringRadius,
                kViewerPerceptionOutlineColor);
        }
    }

    if (flags.influence)
    {
        const float markerPx = kMarkerWorldRadius * camera.pixelsPerUnit;
        for (const InfluenceFieldItem& item : state.GetStaticItems())
        {
            PushFilledCircle(
                list,
                camera.WorldToScreen(item.GetPosition()),
                markerPx,
                ColorForInfluenceType(item.GetInfluenceType()));
        }
        for (const InfluenceFieldItem& item : state.GetStaticAttractors())
        {
            PushFilledCircle(
                list,
                camera.WorldToScreen(item.GetPosition()),
                markerPx,
                ColorForInfluenceType(item.GetInfluenceType()));
        }
    }

    if (flags.obstacles)
    {
        const Map* map = state.GetMap();
        if (map != nullptr)
        {
            for (const auto& entityPtr : map->GetEntities())
            {
                const MapEntity* entity = entityPtr.get();
                if (entity == nullptr)
                {
                    continue;
                }
                const MapObstacleComponent* obstacle = entity->GetComponent<MapObstacleComponent>();
                if (obstacle == nullptr)
                {
                    continue;
                }
                const float radiusPx = obstacle->GetRadius() * camera.pixelsPerUnit;
                PushFilledCircle(
                    list,
                    camera.WorldToScreen(entity->GetPosition()),
                    radiusPx,
                    kViewerObstacleOverlayColor);
            }
        }
    }

    if (flags.behaviorItems)
    {
        const float markerPx = kMarkerWorldRadius * camera.pixelsPerUnit;
        for (IGameUnit* unit : state.GetAllUnits())
        {
            if (unit == nullptr)
            {
                continue;
            }
            auto* host = dynamic_cast<IUnitBehaviorHost*>(unit);
            if (host == nullptr)
            {
                continue;
            }
            IBehavior* behavior = host->GetCurrentBehavior();
            if (behavior == nullptr)
            {
                continue;
            }
            BehaviorContext ctx = state.BuildContextForUnit(unit, 0.0f);
            const std::vector<InfluenceFieldItem> items = behavior->GenerateFieldItems(ctx);
            for (const InfluenceFieldItem& item : items)
            {
                PushFilledCircle(
                    list,
                    camera.WorldToScreen(item.GetPosition()),
                    markerPx,
                    ColorForInfluenceType(item.GetInfluenceType()));
            }
        }
    }

    if (flags.explosions)
    {
        for (const ExplosionEcho& echo : echoes)
        {
            const float radiusPx = echo.radius * camera.pixelsPerUnit;
            PushHollowCircle(
                list,
                camera.WorldToScreen(echo.position),
                radiusPx,
                kViewerExplosionOutlineColor);
        }
    }
}

void AppendStatusLine(
    DrawList& list,
    const GameLogicEngine& engine,
    const PlaybackController& playback,
    const OverlayFlags& flags,
    std::size_t unitCount)
{
    const std::string line = "pause=" + std::string(playback.IsPaused() ? "1" : "0") + " speed="
        + FormatSpeed(engine.GetSpeedMultiplier()) + " overlays=" + OverlayBitsString(flags) + " units="
        + std::to_string(unitCount);
    const Rgba textColor{240, 240, 240, 255};
    DrawText(list, line, 8.0f, 8.0f, textColor);
}

bool SetOverlayFlag(OverlayFlags& flags, const std::string& name, bool enabled, std::string& error)
{
    if (name == "collisions")
    {
        flags.collisions = enabled;
        return true;
    }
    if (name == "perception")
    {
        flags.perception = enabled;
        return true;
    }
    if (name == "influence")
    {
        flags.influence = enabled;
        return true;
    }
    if (name == "obstacles")
    {
        flags.obstacles = enabled;
        return true;
    }
    if (name == "behavior")
    {
        flags.behaviorItems = enabled;
        return true;
    }
    if (name == "explosions")
    {
        flags.explosions = enabled;
        return true;
    }
    error = "unknown overlay name: " + name;
    return false;
}

bool ToggleOverlayFlag(OverlayFlags& flags, const std::string& name)
{
    std::string error;
    if (name == "collisions")
    {
        flags.collisions = !flags.collisions;
        return true;
    }
    if (name == "perception")
    {
        flags.perception = !flags.perception;
        return true;
    }
    if (name == "influence")
    {
        flags.influence = !flags.influence;
        return true;
    }
    if (name == "obstacles")
    {
        flags.obstacles = !flags.obstacles;
        return true;
    }
    if (name == "behavior")
    {
        flags.behaviorItems = !flags.behaviorItems;
        return true;
    }
    if (name == "explosions")
    {
        flags.explosions = !flags.explosions;
        return true;
    }
    return false;
}

OverlayKey OverlayKeyFromViewerFKey(int keyIndex)
{
    switch (keyIndex)
    {
    case 1:
        return OverlayKey::Collisions;
    case 2:
        return OverlayKey::Perception;
    case 3:
        return OverlayKey::Influence;
    case 4:
        return OverlayKey::Obstacles;
    case 5:
        return OverlayKey::BehaviorItems;
    case 6:
        return OverlayKey::Explosions;
    default:
        return OverlayKey::Unknown;
    }
}

