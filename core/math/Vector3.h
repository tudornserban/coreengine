#pragma once

#include <cmath>
#include <algorithm>

struct Vector3 {
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    friend Vector3 operator*(float scalar, const Vector3& v) { return Vector3(v.x * scalar, v.y * scalar, v.z * scalar); }
    Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }
    Vector3& operator+=(const Vector3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vector3& operator-=(const Vector3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vector3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    Vector3 operator-() const { return Vector3(-x, -y, -z); }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }

    Vector3 Normalized() const {
        float len = Length();
        return (len > 0.0f) ? *this / len : Vector3(0.0f, 0.0f, 0.0f);
    }

    void Normalize() {
        float len = Length();
        if (len > 0.0f) { x /= len; y /= len; z /= len; }
    }

    float Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }

    Vector3 Cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    float Distance(const Vector3& other) const { return (*this - other).Length(); }
    float DistanceSquared(const Vector3& other) const { return (*this - other).LengthSquared(); }

    static Vector3 Zero()  { return Vector3(0.0f, 0.0f, 0.0f); }
    static Vector3 One()   { return Vector3(1.0f, 1.0f, 1.0f); }
    static Vector3 UnitX() { return Vector3(1.0f, 0.0f, 0.0f); }
    static Vector3 UnitY() { return Vector3(0.0f, 1.0f, 0.0f); }
    static Vector3 UnitZ() { return Vector3(0.0f, 0.0f, 1.0f); }
    static Vector3 Up()    { return Vector3(0.0f, 1.0f, 0.0f); }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }

    static Vector3 ClampMagnitude(const Vector3& v, float maxLength) {
        float lenSq = v.LengthSquared();
        if (lenSq > maxLength * maxLength) {
            return v * (maxLength / std::sqrt(lenSq));
        }
        return v;
    }
};
