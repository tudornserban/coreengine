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

ViewerKeyAction HandleViewerKeycode(int keycode);
