#pragma once

/// Base interface for all ECS map components.
/// Attach to MapEntity instances to add obstacle, resource,
/// or game-specific data to map entities.
class IMapComponent {
public:
    virtual ~IMapComponent() = default;

    /// Type identifier for safe dynamic component queries.
    virtual const char* GetTypeName() const = 0;
};
