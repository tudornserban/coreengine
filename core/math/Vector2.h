#pragma once

#include <cmath>
#include <algorithm>

struct Vector2 {
    float x, y;

    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    // Operators
    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    friend Vector2 operator*(float scalar, const Vector2& v) {
        return Vector2(v.x * scalar, v.y * scalar);
    }

    Vector2 operator/(float scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    // Utility functions
    float Length() const {
        return std::sqrt(x * x + y * y);
    }

    float LengthSquared() const {
        return x * x + y * y;
    }

    Vector2 Normalized() const {
        float len = Length();
        if (len > 0.0f) {
            return *this / len;
        }
        return Vector2(0.0f, 0.0f);
    }

    void Normalize() {
        float len = Length();
        if (len > 0.0f) {
            x /= len;
            y /= len;
        }
    }

    float Dot(const Vector2& other) const {
        return x * other.x + y * other.y;
    }

    float Distance(const Vector2& other) const {
        return (*this - other).Length();
    }

    float DistanceSquared(const Vector2& other) const {
        return (*this - other).LengthSquared();
    }

    // Static utility functions
    static Vector2 Zero() { return Vector2(0.0f, 0.0f); }
    static Vector2 One() { return Vector2(1.0f, 1.0f); }
    static Vector2 UnitX() { return Vector2(1.0f, 0.0f); }
    static Vector2 UnitY() { return Vector2(0.0f, 1.0f); }

    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
        return a + (b - a) * t;
    }

    static Vector2 Clamp(const Vector2& value, const Vector2& min, const Vector2& max) {
        return Vector2(
            std::max(min.x, std::min(max.x, value.x)),
            std::max(min.y, std::min(max.y, value.y))
        );
    }

    static Vector2 ClampMagnitude(const Vector2& v, float maxLength) {
        float lengthSq = v.LengthSquared();
        if (lengthSq > maxLength * maxLength) {
            float length = std::sqrt(lengthSq);
            return v * (maxLength / length);
        }
        return v;
    }

    Vector2 ProjectOnto(const Vector2& other) const {
        float otherLengthSq = other.LengthSquared();
        if (otherLengthSq < 0.0001f) {
            return Vector2::Zero();
        }
        float projection = this->Dot(other) / otherLengthSq;
        return other * projection;
    }

    // Unary minus operator
    Vector2 operator-() const {
        return Vector2(-x, -y);
    }
};

