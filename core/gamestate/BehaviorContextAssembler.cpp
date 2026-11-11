#include "BehaviorContextAssembler.h"
#include "../interfaces/IGameUnit.h"

BehaviorContext AssembleBehaviorContext(const BehaviorContextInputs& in)
{
    BehaviorContext ctx;
    ctx.self = in.self;
    ctx.deltaTime = in.deltaTime;
    ctx.mapCenter = in.mapCenter;
    ctx.mapSize = in.mapSize;
    ctx.mapBounds = in.mapSize;
    ctx.rng = in.rng;

    Vector2 position = in.self->GetPosition();
    int team = in.self->GetTeam();

    for (IGameUnit* other : in.others)
    {
        if (other == in.self) continue;

        float distance = other->GetPosition().Distance(position);
        if (distance > in.perceptionRadius) continue;

        if (other->GetTeam() == team)
        {
            ctx.nearbyAllies.push_back(other);
        }
        else
        {
            ctx.nearbyEnemies.push_back(other);
        }
    }

    ctx.staticFieldItems = in.mapEdges;
    ctx.staticFieldItems.insert(
        ctx.staticFieldItems.end(),
        in.attractors.begin(),
        in.attractors.end());
    ctx.staticFieldItems.insert(
        ctx.staticFieldItems.end(),
        in.obstacles.begin(),
        in.obstacles.end());

    return ctx;
}
