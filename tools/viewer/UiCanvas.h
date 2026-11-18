#pragma once

#include "DrawList.h"

#include <string_view>

using Rgba8 = Rgba;

void DrawRectFilled(Framebuffer& fb, int x, int y, int w, int h, Rgba8 color);
void DrawRectOutline(Framebuffer& fb, int x, int y, int w, int h, int thickness, Rgba8 color);
void DrawLine(Framebuffer& fb, int x0, int y0, int x1, int y1, Rgba8 color);
void DrawProgressBar(
    Framebuffer& fb,
    int x,
    int y,
    int w,
    int h,
    float fraction,
    Rgba8 fgColor,
    Rgba8 bgColor);
void DrawText5x7(Framebuffer& fb, int x, int y, std::string_view text, int scale, Rgba8 color);
bool HitTestRect(int mouseX, int mouseY, int x, int y, int w, int h);