#pragma once

enum class CollisionType {
    AGENT_AGENT,
    AGENT_MAP_EDGE,
    AGENT_STATIC_OBSTACLE,  // Future
    AGENT_DYNAMIC_OBSTACLE,  // Future
    AGENT_SPAWN_ZONE  // Future
};

