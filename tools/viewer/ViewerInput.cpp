#include "ViewerInput.h"

#include "Overlay.h"

namespace
{

ViewerKey ViewerKeyFromKeycode(int keycode)
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

} // namespace

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

    const ViewerKey viewerKey = ViewerKeyFromKeycode(keycode);
    const PlaybackCommand playback = CommandForKey(viewerKey);
    if (playback != PlaybackCommand::None)
    {
        action.kind = ViewerKeyActionKind::Playback;
        action.playback = playback;
    }
    return action;
}
