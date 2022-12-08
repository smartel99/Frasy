#pragma once


namespace Brigerad
{
/**
 * @class Vec2
 * @brief 2D vector
 *        (Often used to store positions, size, etc.)
 */
struct Vec2
{
    float x, y;
    Vec2()
        : x(0.0f), y(0.0f)
    {
    }
    Vec2(float _x, float _y)
        : x(_x), y(_y)
    {
    }

    Vec2 operator+(const Vec2& other) const
    {
        return Vec2(x + other.x, y + other.y);
    }
    Vec2 operator-(const Vec2& other) const
    {
        return Vec2(x - other.x, y - other.y);
    }
    Vec2 operator*(const Vec2& other) const
    {
        return Vec2(x * other.x, y * other.y);
    }
    Vec2 operator/(const Vec2& other) const
    {
        return Vec2(x / other.x, y / other.y);
    }

    Vec2 operator+(float other) const
    {
        return Vec2(x + other, y + other);
    }
    Vec2 operator-(float other) const
    {
        return Vec2(x - other, y - other);
    }
    Vec2 operator*(float other) const
    {
        return Vec2(x * other, y * other);
    }
    Vec2 operator/(float other) const
    {
        return Vec2(x / other, y / other);
    }

};

/**
 * @class Vec4
 * @brief 4D vector
 *        (often used to store floating-point colors)
 */
struct Vec4
{
    float     x, y, z, w;
    Vec4()
        : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
    {
    }
    Vec4(float _x, float _y, float _z, float _w)
        : x(_x), y(_y), z(_z), w(_w)
    {
    }
};


struct BrBoolVector
{
    std::vector<int>   Storage;
    BrBoolVector()
    {
    }
    void Resize(int sz)
    {
        Storage.resize((sz + 31) >> 5);
        memset(Storage.data(), 0, (size_t)Storage.size() * sizeof(Storage.data()[0]));
    }
    void Clear()
    {
        Storage.clear();
    }
    bool GetBit(int n) const
    {
        int off = (n >> 5);
        int mask = 1 << (n & 31);
        return (Storage[off] & mask) != 0;
    }
    void SetBit(int n, bool v)
    {
        int off = (n >> 5);
        int mask = 1 << (n & 31);
        if (v)
        {
            Storage[off] |= mask;
        }
        else
        {
            Storage[off] &= ~mask;
        }
    }
};
}