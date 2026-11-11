#include "Overlay.h"
#include "PlaybackControl.h"
#include "ViewerInput.h"

#include <gtest/gtest.h>

TEST(ViewerInputTest, PlaybackKeycodesMapToViewerKeysWithSdlLiterals)
{
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(32), ViewerKey::Space);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(49), ViewerKey::Digit1);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(50), ViewerKey::Digit2);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(51), ViewerKey::Digit3);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(52), ViewerKey::Digit4);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(46), ViewerKey::Period);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(114), ViewerKey::R);
    EXPECT_EQ(ViewerKeyFromPlaybackKeycode(999), ViewerKey::Unknown);
}

TEST(ViewerInputTest, OverlayFKeysUseSdlF1LiteralRange)
{
    constexpr int kSdlF1 = 1073741882;
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 0), 1);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 1), 2);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 2), 3);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 3), 4);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 4), 5);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 5), 6);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 - 1), 0);
    EXPECT_EQ(OverlayFKeyIndexFromKeycode(kSdlF1 + 6), 0);
}

TEST(ViewerInputTest, HandleViewerKeycodePassThroughPlaybackAndOverlay)
{
    constexpr int kSdlF1 = 1073741882;

    {
        const ViewerKeyAction action = HandleViewerKeycode(32);
        EXPECT_EQ(action.kind, ViewerKeyActionKind::Playback);
        EXPECT_EQ(action.playback, PlaybackCommand::TogglePause);
    }
    {
        const ViewerKeyAction action = HandleViewerKeycode(51);
        EXPECT_EQ(action.kind, ViewerKeyActionKind::Playback);
        EXPECT_EQ(action.playback, PlaybackCommand::DoubleSpeed);
    }
    {
        const ViewerKeyAction action = HandleViewerKeycode(114);
        EXPECT_EQ(action.kind, ViewerKeyActionKind::Playback);
        EXPECT_EQ(action.playback, PlaybackCommand::ResetScene);
    }
    {
        const ViewerKeyAction action = HandleViewerKeycode(kSdlF1 + 2);
        EXPECT_EQ(action.kind, ViewerKeyActionKind::OverlayToggle);
        EXPECT_EQ(action.overlayToggleIndex, 3);
        EXPECT_EQ(OverlayKeyFromViewerFKey(action.overlayToggleIndex), OverlayKey::Influence);
    }
    {
        const ViewerKeyAction action = HandleViewerKeycode(9999);
        EXPECT_EQ(action.kind, ViewerKeyActionKind::None);
    }
}
