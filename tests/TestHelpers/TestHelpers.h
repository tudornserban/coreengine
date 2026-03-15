#pragma once

#include "core/math/Vector2.h"
#include <cmath>

namespace TestHelpers {

    inline bool FloatEquals(float a, float b, float eps = 1e-4f) {
        return std::fabs(a - b) <= eps;
    }

    inline bool Vector2Equals(const Vector2& a, const Vector2& b, float eps = 1e-4f) {
        return FloatEquals(a.x, b.x, eps) && FloatEquals(a.y, b.y, eps);
    }
}
