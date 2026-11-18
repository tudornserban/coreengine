#include <gtest/gtest.h>

#include "core/map/Map.h"
#include "core/map/MapComponents/MapResourceComponent.h"
#include "core/map/ResourceClaims.h"

TEST(ResourceClaimsTest, LowestIdWinsOnceAndRestoreCanPayAgain) {
    Map map(Vector2(100.0f, 100.0f));
    auto entity = std::make_unique<MapEntity>("ore", Vector2(10.0f, 10.0f));
    entity->AddComponent(std::make_unique<MapResourceComponent>(ResourceType::MINERALS, 12.0f));
    map.AddEntity(std::move(entity));

    const std::vector<ResourceClaim> claims = {
        ResourceClaim{7, "ore"},
        ResourceClaim{3, "ore"},
        ResourceClaim{3, "ore"},
    };
    const std::vector<ResourceGrant> first = ResolveResourceClaims(map, claims);
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(first[0].claimantId, 3u);
    EXPECT_FLOAT_EQ(first[0].amount, 12.0f);
    EXPECT_TRUE(map.GetEntity("ore")->GetComponent<MapResourceComponent>()->IsDepleted());

    const std::vector<ResourceGrant> second = ResolveResourceClaims(map, {ResourceClaim{1, "ore"}});
    EXPECT_TRUE(second.empty());

    map.GetEntity("ore")->GetComponent<MapResourceComponent>()->SetAmount(12.0f);
    const std::vector<ResourceGrant> restored = ResolveResourceClaims(map, {ResourceClaim{9, "ore"}});
    ASSERT_EQ(restored.size(), 1u);
    EXPECT_EQ(restored[0].claimantId, 9u);

    const std::vector<ResourceGrant> reversed = ResolveResourceClaims(map, {
        ResourceClaim{4, "missing"},
    });
    EXPECT_TRUE(reversed.empty());
}

TEST(ResourceClaimsTest, ClaimOrderDoesNotChangeTheWinner) {
    Map map(Vector2(40.0f, 40.0f));
    auto entity = std::make_unique<MapEntity>("well", Vector2::Zero());
    entity->AddComponent(std::make_unique<MapResourceComponent>(ResourceType::ENERGY, 1.0f));
    map.AddEntity(std::move(entity));

    const auto forward = ResolveResourceClaims(map, {ResourceClaim{2, "well"}, ResourceClaim{8, "well"}});
    map.GetEntity("well")->GetComponent<MapResourceComponent>()->SetAmount(1.0f);
    const auto backward = ResolveResourceClaims(map, {ResourceClaim{8, "well"}, ResourceClaim{2, "well"}});
    ASSERT_EQ(forward.size(), 1u);
    ASSERT_EQ(backward.size(), 1u);
    EXPECT_EQ(forward[0].claimantId, backward[0].claimantId);
}
