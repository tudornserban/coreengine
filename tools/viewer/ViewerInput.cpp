#include "ViewerInput.h"

#include "Overlay.h"

int OverlayFKeyIndexFromKeycode(int keycode)
{
    constexpr int kSdlF1 = 1073741882;
    if (keycode >= kSdlF1 && keycode < kSdlF1 + 6)
    {
        return keycode - kSdlF1 + 1;
    }
    return 0;
}

ViewerKey ViewerKeyFromPlaybackKeycode(int keycode)
{
    switch (keycode)
    {
    case 32:
        return ViewerKey::Space;
    case 49:
        return ViewerKey::Digit1;
    case 50:
        return ViewerKey::Digit2;
    case 51:
        return ViewerKey::Digit3;
    case 52:
        return ViewerKey::Digit4;
    case 46:
        return ViewerKey::Period;
    case 114:
        return ViewerKey::R;
    default:
        return ViewerKey::Unknown;
    }
}

ViewerKeyAction HandleViewerKeycode(int keycode)
{
    ViewerKeyAction action;
    const int fKeyIndex = OverlayFKeyIndexFromKeycode(keycode);
    if (fKeyIndex > 0)
    {
        action.kind = ViewerKeyActionKind::OverlayToggle;
        action.overlayToggleIndex = fKeyIndex;
        return action;
    }

    const ViewerKey viewerKey = ViewerKeyFromPlaybackKeycode(keycode);
    const PlaybackCommand playback = CommandForKey(viewerKey);
    if (playback != PlaybackCommand::None)
    {
        action.kind = ViewerKeyActionKind::Playback;
        action.playback = playback;
    }
    return action;
}
