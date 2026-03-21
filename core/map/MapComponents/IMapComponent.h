#pragma once

// Base interface for all map components
// Components can be attached to MapEntity instances
class IMapComponent {
public:
    virtual ~IMapComponent() = default;

    // Component type identifier (for type-safe queries)
    virtual const char* GetTypeName() const = 0;
};

