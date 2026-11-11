#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/ai/BehaviorContext.h"
#include "core/ai/BehaviorFactory.h"
#include "core/influence_types/InfluenceFieldItem.h"
#include "core/interfaces/IBehavior.h"
#include "core/math/Vector2.h"
#include "ffi/c_api.h"

namespace {

constexpr float kStep = 1.0f / 30.0f;
constexpr float kMapW = 1600.0f;
constexpr float kMapH = 900.0f;

uint32_t FloatBits(float value)
{
    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void PushAccel(float /*x*/, float /*y*/, float /*vx*/, float /*vy*/, int /*team*/, float /*dt*/,
               float* outAx, float* outAy, void* /*userdata*/)
{
    *outAx = 30.0f;
    *outAy = -12.0f;
}

void ZeroAccel(float /*x*/, float /*y*/, float /*vx*/, float /*vy*/, int /*team*/, float /*dt*/,
               float* outAx, float* outAy, void* /*userdata*/)
{
    *outAx = 0.0f;
    *outAy = 0.0f;
}

class FieldAttractorBehavior : public IBehavior {
public:
    explicit FieldAttractorBehavior(Vector2 attractor) : attractor_(attractor) {}

    std::vector<InfluenceFieldItem> GenerateFieldItems(const BehaviorContext& /*ctx*/) override
    {
        AttractorParams params;
        params.AttractorStrength = 5000.0f;
        params.functionType = AttractorType::LINEAR;
        return {InfluenceFieldItem::CreateAttractor(attractor_, 1.0f, params)};
    }

    const char* GetName() const override { return "FieldAttractor"; }

    std::map<std::string, float>& GetParameters() override { return params_; }
    const std::map<std::string, float>& GetParameters() const override { return params_; }

private:
    Vector2 attractor_;
    std::map<std::string, float> params_;
};

extern "C" void ThrowingPushAccel(float, float, float, float, int, float, float*, float*, void*)
{
    throw std::runtime_error("callback throw");
}

struct UserdataProbe {
    int calls = 0;
    int expectedTeam = 0;
    float lastDt = -1.0f;
};

void UserdataAccel(float /*x*/, float /*y*/, float /*vx*/, float /*vy*/, int team, float dt,
                   float* outAx, float* outAy, void* userdata)
{
    auto* probe = static_cast<UserdataProbe*>(userdata);
    ++probe->calls;
    probe->expectedTeam = team;
    probe->lastDt = dt;
    *outAx = 1.0f;
    *outAy = 0.0f;
}

class CApiBehaviorTest : public ::testing::Test {
protected:
    CE_Engine e = nullptr;

    void TearDown() override
    {
        BehaviorFactory::Unregister("Push");
        BehaviorFactory::Unregister("PushZero");
        BehaviorFactory::Unregister("UserdataBehavior");
        BehaviorFactory::Unregister("FieldAttractor");
        BehaviorFactory::Unregister("ThrowingPush");
        // Push may already be unregistered in individual tests.
        if (e) {
            ce_engine_destroy(e);
            e = nullptr;
        }
    }
};

}  // namespace

TEST_F(CApiBehaviorTest, LegacyRegisterIsNoOp) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ce_behavior_register("Nope");
    char buf[4096]{};
    ASSERT_GE(ce_behavior_list(buf, static_cast<int>(sizeof(buf))), 0);
    EXPECT_EQ(std::strstr(buf, "Nope"), nullptr);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 100.0f, 100.0f), 1);
    EXPECT_EQ(ce_unit_set_behavior(e, 0, "Nope"), 0);
}

TEST_F(CApiBehaviorTest, RegisterAccelRejectsInvalidArgs) {
    EXPECT_EQ(ce_behavior_register_accel(nullptr, PushAccel, nullptr), 0);
    EXPECT_EQ(ce_behavior_register_accel("", PushAccel, nullptr), 0);
    EXPECT_EQ(ce_behavior_register_accel("Push", nullptr, nullptr), 0);
}

TEST_F(CApiBehaviorTest, CenterUnitStaysStillBeforeBehavior) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(FloatBits(st.x), FloatBits(800.0f));
    EXPECT_EQ(FloatBits(st.y), FloatBits(450.0f));
    EXPECT_EQ(FloatBits(st.vx), FloatBits(0.0f));
    EXPECT_EQ(FloatBits(st.vy), FloatBits(0.0f));
}

TEST_F(CApiBehaviorTest, PushNearLeftEdgeAddsStaticForceToVelocity) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_behavior_register_accel("Push", PushAccel, nullptr), 1);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 20.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "Push"), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(FloatBits(st.vx), 0x3f800031u);
    EXPECT_EQ(FloatBits(st.vy), 0xbeccccceu);
}

TEST_F(CApiBehaviorTest, PushNearTopEdgeAddsStaticForceToVelocity) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_behavior_register_accel("Push", PushAccel, nullptr), 1);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 20.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "Push"), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(FloatBits(st.vx), 0x3f800000u);
    EXPECT_EQ(FloatBits(st.vy), 0xbecccc0bu);
}

TEST_F(CApiBehaviorTest, PushCallbackIntegratesExactLiterals) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_behavior_register_accel("Push", PushAccel, nullptr), 1);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "Push"), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(FloatBits(st.vx), 0x3f800000u);          // 1.0f (0x1p+0)
    EXPECT_EQ(FloatBits(st.vy), 0xbeccccceu);        // -0x1.99999cp-2f (not 0xbecccccd)
    EXPECT_EQ(FloatBits(st.x), 0x44480222u);           // 0x1.900444p+9f (plan table 0x4488011c was wrong)
    EXPECT_EQ(FloatBits(st.y), 0x43e0fe4bu);           // 0x1.c1fc96p+8f (plan table 0x4460fe4b was wrong)
}

TEST_F(CApiBehaviorTest, ReRegisterOverridesCreator) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_behavior_register_accel("Push", PushAccel, nullptr), 1);
    ASSERT_EQ(ce_behavior_register_accel("Push", ZeroAccel, nullptr), 1);

    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "Push"), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(FloatBits(st.vx), FloatBits(0.0f));
    EXPECT_EQ(FloatBits(st.x), FloatBits(800.0f));
}

TEST_F(CApiBehaviorTest, UserdataPassthrough) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    UserdataProbe probe{};
    ASSERT_EQ(ce_behavior_register_accel("UserdataBehavior", UserdataAccel, &probe), 1);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 200.0f, 300.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "UserdataBehavior"), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    EXPECT_EQ(probe.calls, 1);
    EXPECT_EQ(probe.expectedTeam, 1);
    EXPECT_EQ(FloatBits(probe.lastDt), FloatBits(kStep));
}

TEST_F(CApiBehaviorTest, FieldItemsNearLeftEdgeSplitStaticAndBehaviorForces) {
    BehaviorFactory::Register(
        "FieldAttractor",
        [] { return std::make_unique<FieldAttractorBehavior>(Vector2(1500.0f, 450.0f)); });

    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 20.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "FieldAttractor"), 1);
    ASSERT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    // Split hub: behavior field pull capped to 300 then left-edge static adds on X (0x41200007 vs 0x41200000 w/o edges).
    EXPECT_EQ(FloatBits(st.vx), 0x41200007u);
    EXPECT_EQ(FloatBits(st.vy), FloatBits(0.0f));
}

TEST_F(CApiBehaviorTest, UnregisteredBehaviorSetBehaviorFails) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_behavior_register_accel("Push", PushAccel, nullptr), 1);
    BehaviorFactory::Unregister("Push");
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 400.0f, 450.0f), 1);
    EXPECT_EQ(ce_unit_set_behavior(e, 0, "Push"), 0);
}

TEST_F(CApiBehaviorTest, ThrowingCallbackDoesNotEscapeEngineUpdate) {
    e = ce_engine_create(kMapW, kMapH, 1);
    ASSERT_NE(e, nullptr);
    ASSERT_EQ(ce_behavior_register_accel("ThrowingPush", ThrowingPushAccel, nullptr), 1);
    ASSERT_EQ(ce_timer_grant_charge(e, 1), 1);
    ASSERT_EQ(ce_unit_spawn(e, 1, 800.0f, 450.0f), 1);
    ASSERT_EQ(ce_unit_set_behavior(e, 0, "ThrowingPush"), 1);
    EXPECT_EQ(ce_engine_update(e, kStep), 1);

    CE_UnitState st{};
    ASSERT_EQ(ce_unit_get_state(e, 0, &st), 1);
    EXPECT_EQ(FloatBits(st.vx), FloatBits(0.0f));
    EXPECT_EQ(FloatBits(st.vy), FloatBits(0.0f));
}

TEST_F(CApiBehaviorTest, FfiVersionIs130) {
    EXPECT_STREQ(ce_ffi_version(), "1.3.0");
}
