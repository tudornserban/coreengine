#include <gtest/gtest.h>

#include "core/combat/CombatManager.h"
#include "tests/TestHelpers/ProbeUnit.h"

namespace {

CombatManager::Params ZoneParams() {
    CombatManager::Params params;
    params.mode = CombatMode::InfluenceAttrition;
    params.zoneDamagePerSecond = 10.0f;
    params.collisionDamageMultiplier = 1.0f;
    return params;
}

void OverlapPair(ProbeUnit& a, ProbeUnit& b) {
    a.SetInfluenceRadius(20.0f);
    b.SetInfluenceRadius(20.0f);
}

}  // namespace

TEST(CombatManagerTest, ZoneDamageIsSimultaneousAndOrderIndependent) {
    ProbeUnit first(Vector2(0.0f, 0.0f), 1);
    ProbeUnit second(Vector2(10.0f, 0.0f), 2);
    first.SetMaxHP(150.0f);
    second.SetMaxHP(150.0f);
    first.SetHP(150.0f);
    second.SetHP(150.0f);
    OverlapPair(first, second);

    CombatManager::Params params = ZoneParams();
    params.zoneDamagePerSecond = 100.0f;

    CombatManager leftToRight;
    std::vector<IGameUnit*> forward = {&first, &second};
    leftToRight.Resolve(forward, 1.0f, params);

    ProbeUnit third(Vector2(0.0f, 0.0f), 1);
    ProbeUnit fourth(Vector2(10.0f, 0.0f), 2);
    third.SetMaxHP(150.0f);
    fourth.SetMaxHP(150.0f);
    third.SetHP(150.0f);
    fourth.SetHP(150.0f);
    OverlapPair(third, fourth);
    CombatManager rightToLeft;
    std::vector<IGameUnit*> backward = {&fourth, &third};
    rightToLeft.Resolve(backward, 1.0f, params);

    EXPECT_FLOAT_EQ(first.GetCurrentHP(), 50.0f);
    EXPECT_FLOAT_EQ(second.GetCurrentHP(), 50.0f);
    EXPECT_FLOAT_EQ(third.GetCurrentHP(), first.GetCurrentHP());
    EXPECT_FLOAT_EQ(fourth.GetCurrentHP(), second.GetCurrentHP());
}

TEST(CombatManagerTest, LethalExchangeStillDamagesBothUnits) {
    ProbeUnit a(Vector2(0.0f, 0.0f), 1);
    ProbeUnit b(Vector2(5.0f, 0.0f), 2);
    a.SetHP(30.0f);
    b.SetHP(30.0f);
    a.SetMaxHP(30.0f);
    b.SetMaxHP(30.0f);
    OverlapPair(a, b);
    CombatManager::Params params = ZoneParams();
    params.zoneDamagePerSecond = 100.0f;
    CombatManager combat;
    std::vector<IGameUnit*> units = {&a, &b};
    combat.Resolve(units, 1.0f, params);
    EXPECT_TRUE(a.IsDead());
    EXPECT_TRUE(b.IsDead());
}

TEST(CombatManagerTest, OutnumberedProximityCollisionAndThirdTeam) {
    ProbeUnit solo(Vector2(0.0f, 0.0f), 1);
    ProbeUnit extra(Vector2(0.0f, 5.0f), 1);
    ProbeUnit victim(Vector2(10.0f, 0.0f), 2);
    solo.SetInfluenceRadius(30.0f);
    extra.SetInfluenceRadius(30.0f);
    victim.SetInfluenceRadius(30.0f);
    CombatManager::Params params = ZoneParams();
    params.outnumberedBonusPerExtraAttacker = 0.5f;
    CombatManager combat;
    std::vector<IGameUnit*> units = {&solo, &extra, &victim};
    combat.Resolve(units, 1.0f, params);
    EXPECT_FLOAT_EQ(solo.GetCurrentHP(), 90.0f);
    EXPECT_FLOAT_EQ(extra.GetCurrentHP(), 90.0f);
    EXPECT_FLOAT_EQ(victim.GetCurrentHP(), 70.0f);

    ProbeUnit nearA(Vector2(0.0f, 0.0f), 1);
    ProbeUnit nearB(Vector2(0.0f, 0.0f), 2);
    ProbeUnit farA(Vector2(100.0f, 0.0f), 1);
    ProbeUnit farB(Vector2(119.0f, 0.0f), 2);
    for (ProbeUnit* unit : {&nearA, &nearB, &farA, &farB}) {
        unit->SetInfluenceRadius(20.0f);
        unit->SetHP(100.0f);
    }
    CombatManager::Params proximity = ZoneParams();
    proximity.proximityDamageMultiplier = 3.0f;
    std::vector<IGameUnit*> close = {&nearA, &nearB};
    std::vector<IGameUnit*> distant = {&farA, &farB};
    CombatManager proximityCombat;
    proximityCombat.Resolve(close, 1.0f, proximity);
    proximityCombat.Resolve(distant, 1.0f, proximity);
    EXPECT_LT(nearA.GetCurrentHP(), farA.GetCurrentHP());

    ProbeUnit bodyA(Vector2(0.0f, 0.0f), 1, 5.0f);
    ProbeUnit bodyB(Vector2(4.0f, 0.0f), 2, 5.0f);
    ProbeUnit gapA(Vector2(0.0f, 50.0f), 1, 5.0f);
    ProbeUnit gapB(Vector2(12.0f, 50.0f), 2, 5.0f);
    for (ProbeUnit* unit : {&bodyA, &bodyB, &gapA, &gapB}) {
        unit->SetInfluenceRadius(20.0f);
    }
    CombatManager::Params collision = ZoneParams();
    collision.collisionDamageMultiplier = 3.0f;
    std::vector<IGameUnit*> touching = {&bodyA, &bodyB};
    std::vector<IGameUnit*> gapped = {&gapA, &gapB};
    CombatManager collisionCombat;
    collisionCombat.Resolve(touching, 1.0f, collision);
    collisionCombat.Resolve(gapped, 1.0f, collision);
    EXPECT_FLOAT_EQ(bodyA.GetCurrentHP(), 70.0f);
    EXPECT_FLOAT_EQ(gapA.GetCurrentHP(), 90.0f);

    ProbeUnit faction(Vector2(0.0f, 0.0f), 3);
    ProbeUnit other(Vector2(5.0f, 0.0f), 1);
    ProbeUnit bystander(Vector2(400.0f, 0.0f), 2);
    OverlapPair(faction, other);
    bystander.SetInfluenceRadius(20.0f);
    CombatManager::Params teams = ZoneParams();
    std::vector<IGameUnit*> ffa = {&faction, &other, &bystander};
    CombatManager ffaCombat;
    ffaCombat.Resolve(ffa, 1.0f, teams);
    EXPECT_FLOAT_EQ(faction.GetCurrentHP(), 90.0f);
    EXPECT_FLOAT_EQ(bystander.GetCurrentHP(), 100.0f);
}

TEST(CombatManagerTest, AlliesAndAttritionAndDirectAttacks) {
    ProbeUnit a(Vector2(0.0f, 0.0f), 1);
    ProbeUnit b(Vector2(5.0f, 0.0f), 2);
    OverlapPair(a, b);
    CombatManager::Params allied = ZoneParams();
    allied.areAllies = [](const IGameUnit*, const IGameUnit*) { return true; };
    CombatManager combat;
    std::vector<IGameUnit*> units = {&a, &b};
    combat.Resolve(units, 1.0f, allied);
    EXPECT_FLOAT_EQ(a.GetCurrentHP(), 100.0f);

    ProbeUnit surrounded(Vector2(0.0f, 0.0f), 1);
    surrounded.SetMaxHP(100.0f);
    CombatManager attrition;
    attrition.ProcessAttrition(&surrounded, true, 1.0f, 0.25f);
    EXPECT_FLOAT_EQ(surrounded.GetCurrentHP(), 75.0f);
    attrition.ProcessAttrition(&surrounded, false, 1.0f, 0.25f);
    EXPECT_FLOAT_EQ(surrounded.GetCurrentHP(), 75.0f);

    ProbeUnit shooter(Vector2(0.0f, 0.0f), 1);
    ProbeUnit close(Vector2(10.0f, 0.0f), 2);
    ProbeUnit frail(Vector2(40.0f, 0.0f), 2);
    shooter.SetAttackRange(100.0f);
    shooter.SetBaseDPS(10.0f);
    close.SetHP(80.0f);
    frail.SetHP(20.0f);
    CombatManager::Params nearest = ZoneParams();
    nearest.mode = CombatMode::DirectAttack;
    nearest.zoneDamagePerSecond = 0.0f;
    nearest.targetPolicy = TargetPolicy::Nearest;
    std::vector<IGameUnit*> direct = {&shooter, &close, &frail};
    CombatManager directCombat;
    directCombat.Resolve(direct, 1.0f, nearest);
    EXPECT_FLOAT_EQ(close.GetCurrentHP(), 70.0f);
    EXPECT_FLOAT_EQ(frail.GetCurrentHP(), 20.0f);

    close.SetHP(80.0f);
    frail.SetHP(20.0f);
    CombatManager::Params lowest = nearest;
    lowest.targetPolicy = TargetPolicy::LowestHP;
    CombatManager lowestCombat;
    lowestCombat.Resolve(direct, 1.0f, lowest);
    EXPECT_FLOAT_EQ(close.GetCurrentHP(), 80.0f);
    EXPECT_FLOAT_EQ(frail.GetCurrentHP(), 10.0f);

    ProbeUnit threatShooter(Vector2(0.0f, 0.0f), 1);
    ProbeUnit weak(Vector2(10.0f, 0.0f), 2);
    ProbeUnit strong(Vector2(30.0f, 0.0f), 2);
    threatShooter.SetAttackRange(100.0f);
    threatShooter.SetBaseDPS(5.0f);
    weak.SetBaseDPS(1.0f);
    strong.SetBaseDPS(9.0f);
    CombatManager::Params threat = nearest;
    threat.targetPolicy = TargetPolicy::HighestThreat;
    std::vector<IGameUnit*> threats = {&threatShooter, &weak, &strong};
    CombatManager threatCombat;
    threatCombat.Resolve(threats, 1.0f, threat);
    EXPECT_FLOAT_EQ(weak.GetCurrentHP(), 100.0f);
    EXPECT_FLOAT_EQ(strong.GetCurrentHP(), 95.0f);
}

TEST(CombatManagerTest, CooldownSplashMultiplierAndHybrid) {
    ProbeUnit shooter(Vector2(0.0f, 0.0f), 1);
    ProbeUnit target(Vector2(10.0f, 0.0f), 2);
    ProbeUnit splash(Vector2(25.0f, 0.0f), 2);
    ProbeUnit far(Vector2(80.0f, 0.0f), 2);
    shooter.SetAttackRange(100.0f);
    shooter.SetBaseDPS(10.0f);
    shooter.SetAttackCooldown(1.0f);
    CombatManager::Params params;
    params.mode = CombatMode::DirectAttack;
    params.splashRadius = 30.0f;
    params.splashFalloff = 0.0f;
    params.damageMultiplier = [](const IGameUnit*, const IGameUnit*) { return 2.0f; };
    std::vector<IGameUnit*> units = {&shooter, &target, &splash, &far};
    CombatManager combat;
    combat.Resolve(units, 0.4f, params);
    EXPECT_FLOAT_EQ(target.GetCurrentHP(), 100.0f);
    combat.Resolve(units, 0.6f, params);
    EXPECT_FLOAT_EQ(target.GetCurrentHP(), 80.0f);
    EXPECT_FLOAT_EQ(splash.GetCurrentHP(), 80.0f);
    EXPECT_FLOAT_EQ(far.GetCurrentHP(), 100.0f);

    ProbeUnit beam(Vector2(0.0f, 0.0f), 1);
    ProbeUnit primary(Vector2(10.0f, 0.0f), 2);
    ProbeUnit half(Vector2(25.0f, 0.0f), 2);
    beam.SetAttackRange(20.0f);
    beam.SetBaseDPS(10.0f);
    CombatManager::Params falloff;
    falloff.mode = CombatMode::DirectAttack;
    falloff.splashRadius = 30.0f;
    falloff.splashFalloff = 1.0f;
    std::vector<IGameUnit*> splashUnits = {&beam, &primary, &half};
    CombatManager falloffCombat;
    falloffCombat.Resolve(splashUnits, 1.0f, falloff);
    EXPECT_FLOAT_EQ(primary.GetCurrentHP(), 90.0f);
    EXPECT_FLOAT_EQ(half.GetCurrentHP(), 95.0f);

    ProbeUnit left(Vector2(0.0f, 0.0f), 1);
    ProbeUnit right(Vector2(10.0f, 0.0f), 2);
    OverlapPair(left, right);
    left.SetAttackRange(30.0f);
    left.SetBaseDPS(5.0f);
    CombatManager::Params hybrid = ZoneParams();
    hybrid.mode = CombatMode::Hybrid;
    std::vector<IGameUnit*> both = {&left, &right};
    CombatManager hybridCombat;
    hybridCombat.Resolve(both, 1.0f, hybrid);
    EXPECT_FLOAT_EQ(right.GetCurrentHP(), 85.0f);
    EXPECT_FLOAT_EQ(left.GetCurrentHP(), 90.0f);
}
