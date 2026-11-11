#pragma once

#include "PlaybackControl.h"

enum class ViewerKeyActionKind
{
    None,
    OverlayToggle,
    Playback
};

struct ViewerKeyAction
{
    ViewerKeyActionKind kind = ViewerKeyActionKind::None;
    int overlayToggleIndex = 0;
    PlaybackCommand playback = PlaybackCommand::None;
};

/// SDL keysym-style integers (e.g. 32 = space, 1073741882 = F1); no SDL headers required.
ViewerKey ViewerKeyFromPlaybackKeycode(int keycode);
int OverlayFKeyIndexFromKeycode(int keycode);
ViewerKeyAction HandleViewerKeycode(int keycode);
