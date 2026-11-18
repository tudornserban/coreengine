#include "RTSGameState.h"

#include "RTSUnit.h"
#include "core/ai/StateMachine.h"
#include "core/interfaces/IBehavior.h"
#include "core/map/MapComponents/MapResourceComponent.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr int kSchemaVersion = 1;

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const char* message) : std::runtime_error(message) {}
};

std::string FormatFloat(float value) {
    std::ostringstream out;
    out << std::hexfloat << value;
    return out.str();
}

float ParseFloat(const std::string& token) {
    std::size_t used = 0;
    const float value = std::stof(token, &used);
    if (used != token.size()) {
        throw ParseError("float");
    }
    return value;
}

uint64_t ParseU64(const std::string& token) {
    std::size_t used = 0;
    const unsigned long long value = std::stoull(token, &used, 10);
    if (used != token.size()) {
        throw ParseError("u64");
    }
    return static_cast<uint64_t>(value);
}

int ParseInt(const std::string& token) {
    std::size_t used = 0;
    const int value = std::stoi(token, &used, 10);
    if (used != token.size()) {
        throw ParseError("int");
    }
    return value;
}

std::vector<std::string> Tokens(const std::string& line) {
    std::istringstream in(line);
    std::vector<std::string> tokens;
    std::string token;
    while (in >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string HexBytes(const std::vector<uint8_t>& bytes) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.resize(bytes.size() * 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        out[i * 2] = digits[(bytes[i] >> 4) & 0x0f];
        out[i * 2 + 1] = digits[bytes[i] & 0x0f];
    }
    return out;
}

std::vector<uint8_t> Unhex(const std::string& text) {
    if (text.size() % 2 != 0) {
        throw ParseError("hex");
    }
    std::vector<uint8_t> bytes;
    bytes.reserve(text.size() / 2);
    auto nibble = [](char ch) {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        throw ParseError("hex");
    };
    for (std::size_t i = 0; i < text.size(); i += 2) {
        bytes.push_back(static_cast<uint8_t>((nibble(text[i]) << 4) | nibble(text[i + 1])));
    }
    return bytes;
}

uint64_t HashText(const std::string& text) {
    uint64_t hash = 14695981039346656037ull;
    for (unsigned char byte : text) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    return hash;
}

struct BehaviorBlob {
    std::string name;
    std::vector<std::pair<std::string, float>> parameters;
};

struct TransitionBlob {
    int hasState = 0;
    float value = 0.0f;
};

struct UnitBlob {
    uint64_t id = 0;
    int team = 0;
    float x = 0, y = 0, vx = 0, vy = 0, hp = 0, maxHp = 0, radius = 0;
    float maxSpeed = 0, maxAccel = 0, baseMass = 0, massMod = 0;
    int movement = 1, action = 1, collision = 1, deathLatched = 0;
    std::string currentState = "-";
    std::vector<BehaviorBlob> behaviors;
    std::vector<TransitionBlob> transitions;
};

struct ProjectileBlob {
    uint64_t id = 0, ownerId = 0;
    int team = 0;
    float x = 0, y = 0, vx = 0, vy = 0, radius = 0, timeRemaining = 0, damage = 0;
    int hitAllies = 0;
};

struct ActionBlob {
    uint64_t id = 0, entityId = 0;
    uint32_t duration = 0, elapsed = 0;
    int status = 0;
};

struct ParsedSnapshot {
    int schema = 0;
    std::string engine;
    std::string rules;
    uint32_t tick = 0;
    uint64_t rngSeed = 0, rngState = 0, nextId = 1;
    uint64_t nextProjectile = 1, nextAction = 1;
    std::vector<LifecycleEvent> lifecycle;
    std::vector<UnitBlob> units;
    std::vector<ProjectileBlob> projectiles;
    std::vector<ActionBlob> actions;
    std::vector<CombatManager::CooldownRecord> cooldowns;
    std::vector<std::pair<std::string, float>> resources;
    struct ImmuneLink {
        uint64_t unit = 0;
        int frames = 0;
        uint64_t against = 0;
    };
    std::vector<ImmuneLink> immunity;
    std::vector<uint8_t> extension;
};

std::string Require(const std::vector<std::string>& tokens, std::size_t index) {
    if (index >= tokens.size()) {
        throw ParseError("missing");
    }
    return tokens[index];
}

ParsedSnapshot ParseSnapshot(const std::string& blob) {
    std::istringstream in(blob);
    std::string line;
    if (!std::getline(in, line)) {
        throw ParseError("empty");
    }
    const auto header = Tokens(line);
    if (header.size() != 2 || header[0] != "CESTATE") {
        throw ParseError("magic");
    }
    ParsedSnapshot parsed;
    parsed.schema = ParseInt(header[1]);
    if (parsed.schema != kSchemaVersion) {
        throw ParseError("schema");
    }

    auto next = [&]() {
        if (!std::getline(in, line)) {
            throw ParseError("truncated");
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return Tokens(line);
    };

    auto expect = [&](const char* key) {
        const auto tokens = next();
        if (tokens.size() < 2 || tokens[0] != key) {
            throw ParseError(key);
        }
        return tokens;
    };

    parsed.engine = Require(expect("engine"), 1);
    if (parsed.engine != "coreengine") {
        throw ParseError("engine");
    }
    parsed.rules = Require(expect("rules"), 1);
    if (ParseInt(Require(expect("schema"), 1)) != kSchemaVersion) {
        throw ParseError("schema");
    }
    parsed.tick = static_cast<uint32_t>(ParseU64(Require(expect("tick"), 1)));
    parsed.rngSeed = ParseU64(Require(expect("rng_seed"), 1));
    parsed.rngState = ParseU64(Require(expect("rng_state"), 1));
    parsed.nextId = ParseU64(Require(expect("next_id"), 1));
    parsed.nextProjectile = ParseU64(Require(expect("next_projectile"), 1));
    parsed.nextAction = ParseU64(Require(expect("next_action"), 1));

    const std::size_t lifeCount = static_cast<std::size_t>(ParseU64(Require(expect("lifecycle"), 1)));
    for (std::size_t i = 0; i < lifeCount; ++i) {
        const auto tokens = next();
        if (tokens.size() != 3 || tokens[0] != "death") {
            throw ParseError("death");
        }
        LifecycleEvent event;
        event.kind = LifecycleEventKind::Died;
        event.entityId = ParseU64(tokens[1]);
        event.tick = static_cast<uint32_t>(ParseU64(tokens[2]));
        parsed.lifecycle.push_back(event);
    }

    const std::size_t unitCount = static_cast<std::size_t>(ParseU64(Require(expect("units"), 1)));
    for (std::size_t i = 0; i < unitCount; ++i) {
        const auto tokens = next();
        if (tokens.size() != 17 || tokens[0] != "unit") {
            throw ParseError("unit");
        }
        UnitBlob unit;
        unit.id = ParseU64(tokens[1]);
        unit.team = ParseInt(tokens[2]);
        unit.x = ParseFloat(tokens[3]);
        unit.y = ParseFloat(tokens[4]);
        unit.vx = ParseFloat(tokens[5]);
        unit.vy = ParseFloat(tokens[6]);
        unit.hp = ParseFloat(tokens[7]);
        unit.maxHp = ParseFloat(tokens[8]);
        unit.radius = ParseFloat(tokens[9]);
        unit.maxSpeed = ParseFloat(tokens[10]);
        unit.maxAccel = ParseFloat(tokens[11]);
        unit.baseMass = ParseFloat(tokens[12]);
        unit.massMod = ParseFloat(tokens[13]);
        unit.movement = ParseInt(tokens[14]);
        unit.action = ParseInt(tokens[15]);
        unit.collision = ParseInt(tokens[16]);
        const auto latchLine = next();
        if (latchLine.size() != 2 || latchLine[0] != "latch") {
            throw ParseError("latch");
        }
        unit.deathLatched = ParseInt(latchLine[1]);
        const auto stateLine = next();
        if (stateLine.size() != 2 || stateLine[0] != "behavior") {
            throw ParseError("behavior");
        }
        unit.currentState = stateLine[1];
        const std::size_t stateCount = static_cast<std::size_t>(ParseU64(Require(next(), 1)));
        if (Tokens(line)[0] != "states") {
            throw ParseError("states");
        }
        for (std::size_t s = 0; s < stateCount; ++s) {
            const auto nameLine = next();
            if (nameLine.size() != 2 || nameLine[0] != "state") {
                throw ParseError("state");
            }
            BehaviorBlob behavior;
            behavior.name = nameLine[1];
            const std::size_t paramCount = static_cast<std::size_t>(ParseU64(Require(next(), 1)));
            if (Tokens(line)[0] != "params") {
                throw ParseError("params");
            }
            for (std::size_t p = 0; p < paramCount; ++p) {
                const auto param = next();
                if (param.size() != 3 || param[0] != "param") {
                    throw ParseError("param");
                }
                behavior.parameters.emplace_back(param[1], ParseFloat(param[2]));
            }
            unit.behaviors.push_back(std::move(behavior));
        }
        const std::size_t transitionCount = static_cast<std::size_t>(ParseU64(Require(next(), 1)));
        if (Tokens(line)[0] != "transitions") {
            throw ParseError("transitions");
        }
        for (std::size_t t = 0; t < transitionCount; ++t) {
            const auto transition = next();
            if (transition.size() != 3 || transition[0] != "transition") {
                throw ParseError("transition");
            }
            unit.transitions.push_back(TransitionBlob{ParseInt(transition[1]), ParseFloat(transition[2])});
        }
        parsed.units.push_back(std::move(unit));
    }

    const auto loadCount = [&](const char* key, const char* row, auto&& fill) {
        const std::size_t count = static_cast<std::size_t>(ParseU64(Require(expect(key), 1)));
        for (std::size_t i = 0; i < count; ++i) {
            const auto tokens = next();
            if (tokens.empty() || tokens[0] != row) {
                throw ParseError(row);
            }
            fill(tokens);
        }
    };

    loadCount("projectiles", "projectile", [&](const std::vector<std::string>& tokens) {
        if (tokens.size() != 12) throw ParseError("projectile");
        ProjectileBlob projectile;
        projectile.id = ParseU64(tokens[1]);
        projectile.ownerId = ParseU64(tokens[2]);
        projectile.team = ParseInt(tokens[3]);
        projectile.x = ParseFloat(tokens[4]);
        projectile.y = ParseFloat(tokens[5]);
        projectile.vx = ParseFloat(tokens[6]);
        projectile.vy = ParseFloat(tokens[7]);
        projectile.radius = ParseFloat(tokens[8]);
        projectile.timeRemaining = ParseFloat(tokens[9]);
        projectile.damage = ParseFloat(tokens[10]);
        projectile.hitAllies = ParseInt(tokens[11]);
        parsed.projectiles.push_back(projectile);
    });
    loadCount("actions", "action", [&](const std::vector<std::string>& tokens) {
        if (tokens.size() != 6) throw ParseError("action");
        ActionBlob action;
        action.id = ParseU64(tokens[1]);
        action.entityId = ParseU64(tokens[2]);
        action.duration = static_cast<uint32_t>(ParseU64(tokens[3]));
        action.elapsed = static_cast<uint32_t>(ParseU64(tokens[4]));
        action.status = ParseInt(tokens[5]);
        parsed.actions.push_back(action);
    });
    loadCount("cooldowns", "cooldown", [&](const std::vector<std::string>& tokens) {
        if (tokens.size() != 3) throw ParseError("cooldown");
        parsed.cooldowns.push_back(
            CombatManager::CooldownRecord{ParseU64(tokens[1]), ParseFloat(tokens[2])});
    });
    loadCount("resources", "resource", [&](const std::vector<std::string>& tokens) {
        if (tokens.size() != 3) throw ParseError("resource");
        parsed.resources.emplace_back(tokens[1], ParseFloat(tokens[2]));
    });
    loadCount("immunity", "immune", [&](const std::vector<std::string>& tokens) {
        if (tokens.size() != 4) throw ParseError("immune");
        parsed.immunity.push_back(
            ParsedSnapshot::ImmuneLink{ParseU64(tokens[1]), ParseInt(tokens[2]), ParseU64(tokens[3])});
    });

    const auto extension = next();
    if (extension.empty() || extension[0] != "extension") {
        throw ParseError("extension");
    }
    if (extension.size() == 2) {
        parsed.extension = Unhex(extension[1]);
    } else if (extension.size() != 1) {
        throw ParseError("extension");
    }
    const auto end = next();
    if (end.size() != 1 || end[0] != "END") {
        throw ParseError("end");
    }
    return parsed;
}

}  // namespace

std::string RTSGameState::CaptureCanonicalState() const {
    std::ostringstream out;
    out << "CESTATE " << kSchemaVersion << "\n";
    out << "engine coreengine\n";
    out << "rules " << (rulesIdentity_.empty() ? std::string("unspecified") : rulesIdentity_) << "\n";
    out << "schema " << kSchemaVersion << "\n";
    out << "tick " << simulationTick_ << "\n";
    out << "rng_seed " << rng_.GetSeed() << "\n";
    out << "rng_state " << rng_.GetState() << "\n";
    out << "next_id " << nextEntityId_ << "\n";
    out << "next_projectile " << projectiles_.NextId() << "\n";
    out << "next_action " << actions_.NextId() << "\n";
    out << "lifecycle " << lifecycleEvents_.size() << "\n";
    for (const LifecycleEvent& event : lifecycleEvents_) {
        out << "death " << event.entityId << " " << event.tick << "\n";
    }

    std::vector<const IGameUnit*> ordered;
    ordered.reserve(units_.size());
    for (const auto& unit : units_) {
        ordered.push_back(unit.get());
    }
    std::sort(ordered.begin(), ordered.end(), [&](const IGameUnit* a, const IGameUnit* b) {
        return GetEntityId(a) < GetEntityId(b);
    });
    out << "units " << ordered.size() << "\n";
    for (const IGameUnit* unit : ordered) {
        const auto* rts = dynamic_cast<const RTSUnit*>(unit);
        const float maxAccel = unit->GetMaxAcceleration();
        out << "unit " << GetEntityId(unit)
            << " " << unit->GetTeam()
            << " " << FormatFloat(unit->GetPosition().x)
            << " " << FormatFloat(unit->GetPosition().y)
            << " " << FormatFloat(unit->GetVelocity().x)
            << " " << FormatFloat(unit->GetVelocity().y)
            << " " << FormatFloat(unit->GetCurrentHP())
            << " " << FormatFloat(unit->GetMaxHP())
            << " " << FormatFloat(unit->GetRadius())
            << " " << FormatFloat(unit->GetMaxSpeed())
            << " " << FormatFloat(maxAccel)
            << " " << FormatFloat(unit->GetBaseMass())
            << " " << FormatFloat(unit->GetMassModifier())
            << " " << (unit->ParticipatesInMovement() ? 1 : 0)
            << " " << (unit->ParticipatesInActions() ? 1 : 0)
            << " " << (unit->ParticipatesInCollision() ? 1 : 0)
            << "\n";
        out << "latch " << (rts && rts->IsDeathLatched() ? 1 : 0) << "\n";
        std::string current = "-";
        if (rts && !rts->GetStateMachine().GetCurrentState().empty()) {
            current = rts->GetStateMachine().GetCurrentState();
        }
        out << "behavior " << current << "\n";
        if (!rts) {
            out << "states 0\ntransitions 0\n";
            continue;
        }
        const auto& states = rts->GetStateMachine().GetStates();
        out << "states " << states.size() << "\n";
        for (const auto& state : states) {
            out << "state " << state.first << "\n";
            std::vector<std::pair<std::string, float>> parameters;
            if (state.second) {
                for (const auto& parameter : state.second->GetParameters()) {
                    parameters.push_back(parameter);
                }
            }
            std::sort(parameters.begin(), parameters.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });
            out << "params " << parameters.size() << "\n";
            for (const auto& parameter : parameters) {
                out << "param " << parameter.first << " " << FormatFloat(parameter.second) << "\n";
            }
        }
        const StateMachine& machine = rts->GetStateMachine();
        out << "transitions " << machine.TransitionCount() << "\n";
        for (std::size_t i = 0; i < machine.TransitionCount(); ++i) {
            out << "transition " << (machine.TransitionHasNumericState(i) ? 1 : 0)
                << " " << FormatFloat(machine.TransitionNumericState(i)) << "\n";
        }
    }

    std::vector<Projectile> projectiles = projectiles_.Projectiles();
    std::sort(projectiles.begin(), projectiles.end(),
              [](const Projectile& a, const Projectile& b) { return a.id < b.id; });
    out << "projectiles " << projectiles.size() << "\n";
    for (const Projectile& projectile : projectiles) {
        out << "projectile " << projectile.id << " " << projectile.ownerId << " " << projectile.team
            << " " << FormatFloat(projectile.position.x) << " " << FormatFloat(projectile.position.y)
            << " " << FormatFloat(projectile.velocity.x) << " " << FormatFloat(projectile.velocity.y)
            << " " << FormatFloat(projectile.radius) << " " << FormatFloat(projectile.timeRemaining)
            << " " << FormatFloat(projectile.damage) << " " << (projectile.hitAllies ? 1 : 0) << "\n";
    }

    std::vector<TimedAction> timed = actions_.Capture();
    std::sort(timed.begin(), timed.end(), [](const TimedAction& a, const TimedAction& b) { return a.id < b.id; });
    out << "actions " << timed.size() << "\n";
    for (const TimedAction& action : timed) {
        out << "action " << action.id << " " << action.entityId << " " << action.durationTicks
            << " " << action.elapsedTicks << " " << static_cast<int>(action.status) << "\n";
    }

    std::vector<CombatManager::CooldownRecord> cooldowns;
    if (combat_) {
        cooldowns = combat_->ExportCooldowns();
    }
    out << "cooldowns " << cooldowns.size() << "\n";
    for (const auto& cooldown : cooldowns) {
        out << "cooldown " << cooldown.entityId << " " << FormatFloat(cooldown.remaining) << "\n";
    }

    std::vector<std::pair<std::string, float>> resources;
    if (map_) {
        for (const auto& entity : map_->GetEntities()) {
            if (const auto* resource = entity->GetComponent<MapResourceComponent>()) {
                resources.emplace_back(entity->GetId(), resource->GetAmount());
            }
        }
    }
    std::sort(resources.begin(), resources.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    out << "resources " << resources.size() << "\n";
    for (const auto& resource : resources) {
        out << "resource " << resource.first << " " << FormatFloat(resource.second) << "\n";
    }

    struct ImmuneLine {
        uint64_t unit;
        int frames;
        uint64_t against;
    };
    std::vector<ImmuneLine> immuneLines;
    for (const auto& record : collisionSystem_.ExportImmunity()) {
        const uint64_t unitId = GetEntityId(record.unit);
        for (IGameUnit* other : record.against) {
            immuneLines.push_back(ImmuneLine{unitId, record.remainingFrames, GetEntityId(other)});
        }
    }
    std::sort(immuneLines.begin(), immuneLines.end(), [](const ImmuneLine& a, const ImmuneLine& b) {
        if (a.unit != b.unit) return a.unit < b.unit;
        return a.against < b.against;
    });
    out << "immunity " << immuneLines.size() << "\n";
    for (const ImmuneLine& line : immuneLines) {
        out << "immune " << line.unit << " " << line.frames << " " << line.against << "\n";
    }

    std::vector<uint8_t> extension;
    if (extensionCapture_) {
        extension = extensionCapture_();
    }
    out << "extension " << HexBytes(extension) << "\n";
    out << "END\n";
    return out.str();
}

uint64_t RTSGameState::CanonicalStateHash() const {
    return HashText(CaptureCanonicalState());
}

bool RTSGameState::RestoreCanonicalState(const std::string& blob) {
    ParsedSnapshot parsed;
    try {
        parsed = ParseSnapshot(blob);
    } catch (const std::exception&) {
        return false;
    }
    if (parsed.rules != rulesIdentity_ && !(parsed.rules == "unspecified" && rulesIdentity_.empty())) {
        if (parsed.rules != rulesIdentity_) {
            return false;
        }
    }

    std::unordered_map<uint64_t, IGameUnit*> byId;
    for (const auto& unit : units_) {
        const uint64_t id = GetEntityId(unit.get());
        if (id == 0 || byId.count(id) != 0) {
            return false;
        }
        byId.emplace(id, unit.get());
    }
    if (byId.size() != parsed.units.size()) {
        return false;
    }

    for (const UnitBlob& blobUnit : parsed.units) {
        const auto found = byId.find(blobUnit.id);
        if (found == byId.end()) {
            return false;
        }
        auto* rts = dynamic_cast<RTSUnit*>(found->second);
        if (!rts) {
            return false;
        }
        if (blobUnit.currentState != "-" &&
            rts->GetStateMachine().GetStates().count(blobUnit.currentState) == 0) {
            return false;
        }
        if (blobUnit.behaviors.size() != rts->GetStateMachine().GetStates().size()) {
            return false;
        }
        for (const BehaviorBlob& behavior : blobUnit.behaviors) {
            if (rts->GetStateMachine().GetStates().count(behavior.name) == 0) {
                return false;
            }
            const IBehavior* live = rts->GetStateMachine().GetStates().at(behavior.name).get();
            if (!live) {
                return false;
            }
            for (const auto& parameter : behavior.parameters) {
                if (live->GetParameters().count(parameter.first) == 0) {
                    return false;
                }
            }
        }
        if (blobUnit.transitions.size() != rts->GetStateMachine().TransitionCount()) {
            return false;
        }
    }

    if (extensionValidate_) {
        if (!extensionValidate_(parsed.extension)) {
            return false;
        }
    } else if (!parsed.extension.empty()) {
        return false;
    }
    if (!combat_ && !parsed.cooldowns.empty()) {
        return false;
    }
    if (map_) {
        for (const auto& resource : parsed.resources) {
            MapEntity* entity = map_->GetEntity(resource.first);
            if (!entity || !entity->GetComponent<MapResourceComponent>()) {
                return false;
            }
        }
    } else if (!parsed.resources.empty()) {
        return false;
    }
    for (const auto& link : parsed.immunity) {
        if (byId.count(link.unit) == 0 || byId.count(link.against) == 0) {
            return false;
        }
    }

    for (const UnitBlob& blobUnit : parsed.units) {
        auto* rts = dynamic_cast<RTSUnit*>(byId.at(blobUnit.id));
        rts->SetPosition(Vector2(blobUnit.x, blobUnit.y));
        rts->SetVelocity(Vector2(blobUnit.vx, blobUnit.vy));
        rts->SetMaxHP(blobUnit.maxHp, MaxHpPolicy::ClampCurrent);
        rts->SetHP(blobUnit.hp);
        rts->SetRadius(blobUnit.radius);
        rts->SetMaxSpeed(blobUnit.maxSpeed);
        rts->SetMaxAcceleration(blobUnit.maxAccel);
        rts->SetBaseMass(blobUnit.baseMass);
        rts->SetMassModifier(blobUnit.massMod);
        rts->SetMovementParticipation(blobUnit.movement != 0);
        rts->SetActionParticipation(blobUnit.action != 0);
        rts->SetCollisionParticipation(blobUnit.collision != 0);
        rts->RestoreDeathLatch(blobUnit.deathLatched != 0);
        for (const BehaviorBlob& behavior : blobUnit.behaviors) {
            if (!rts->GetStateMachine().RestoreBehaviorParameters(behavior.name, behavior.parameters)) {
                return false;
            }
        }
        const std::string stateName = blobUnit.currentState == "-" ? std::string() : blobUnit.currentState;
        if (!rts->GetStateMachine().RestoreCurrentStateName(stateName)) {
            return false;
        }
        for (std::size_t i = 0; i < blobUnit.transitions.size(); ++i) {
            if (blobUnit.transitions[i].hasState) {
                rts->GetStateMachine().SetTransitionNumericState(i, blobUnit.transitions[i].value);
            }
        }
    }

    std::vector<Projectile> projectiles;
    for (const ProjectileBlob& blobProjectile : parsed.projectiles) {
        Projectile projectile;
        projectile.id = blobProjectile.id;
        projectile.ownerId = blobProjectile.ownerId;
        projectile.team = blobProjectile.team;
        projectile.position = Vector2(blobProjectile.x, blobProjectile.y);
        projectile.velocity = Vector2(blobProjectile.vx, blobProjectile.vy);
        projectile.radius = blobProjectile.radius;
        projectile.timeRemaining = blobProjectile.timeRemaining;
        projectile.damage = blobProjectile.damage;
        projectile.hitAllies = blobProjectile.hitAllies != 0;
        projectiles.push_back(projectile);
    }
    projectiles_.Restore(std::move(projectiles), parsed.nextProjectile);

    std::vector<TimedAction> actions;
    for (const ActionBlob& blobAction : parsed.actions) {
        TimedAction action;
        action.id = blobAction.id;
        action.entityId = blobAction.entityId;
        action.durationTicks = blobAction.duration;
        action.elapsedTicks = blobAction.elapsed;
        action.status = static_cast<TimedActionStatus>(blobAction.status);
        actions.push_back(action);
    }
    actions_.Restore(std::move(actions), parsed.nextAction);

    if (combat_) {
        combat_->ImportCooldowns(parsed.cooldowns);
    } else if (!parsed.cooldowns.empty()) {
        return false;
    }

    if (map_) {
        for (const auto& resource : parsed.resources) {
            MapEntity* entity = map_->GetEntity(resource.first);
            if (!entity) {
                return false;
            }
            auto* component = entity->GetComponent<MapResourceComponent>();
            if (!component) {
                return false;
            }
            component->SetAmount(resource.second);
        }
    }

    std::unordered_map<uint64_t, IGameUnit*> idToUnit = byId;
    std::vector<ImmunityManager::ExportRecord> immunity;
    std::map<IGameUnit*, ImmunityManager::ExportRecord> grouped;
    for (const auto& link : parsed.immunity) {
        IGameUnit* unit = idToUnit.count(link.unit) ? idToUnit[link.unit] : nullptr;
        IGameUnit* against = idToUnit.count(link.against) ? idToUnit[link.against] : nullptr;
        if (!unit || !against) {
            return false;
        }
        auto& record = grouped[unit];
        record.unit = unit;
        record.remainingFrames = link.frames;
        record.against.push_back(against);
    }
    for (auto& entry : grouped) {
        immunity.push_back(std::move(entry.second));
    }
    collisionSystem_.ImportImmunity(immunity);

    simulationTick_ = parsed.tick;
    rng_.SetSeed(parsed.rngSeed);
    rng_.SetState(parsed.rngState);
    nextEntityId_ = parsed.nextId;
    lifecycleEvents_ = parsed.lifecycle;
    rulesIdentity_ = parsed.rules;
    if (extensionApply_) {
        extensionApply_(parsed.extension);
    }
    return true;
}
