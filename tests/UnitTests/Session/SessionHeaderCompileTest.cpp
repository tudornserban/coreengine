#include <gtest/gtest.h>

#include "core/session/IDeterministicMatch.h"
#include "core/session/ISessionTransport.h"
#include "core/session/InProcessHost.h"
#include "core/session/Latency.h"
#include "core/session/OfflineResolver.h"
#include "core/session/OnlineMatch.h"
#include "core/session/SessionClient.h"
#include "core/session/SessionCodec.h"
#include "core/session/SessionServer.h"
#include "core/session/SessionTypes.h"

TEST(SessionHeaderCompile, PublicHeadersExposeLobbyAndBothPlayModes)
{
    EXPECT_EQ(Session::ToString(Session::GameTitle::DynamicGo)[0], 'D');
    EXPECT_EQ(Session::InputDelayTicksForRtt(0, Session::LatencyProfile{}), 1u);
    const Session::MatchOutcome* outcome = nullptr;
    EXPECT_EQ(outcome, nullptr);
}
