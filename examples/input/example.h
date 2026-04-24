#pragma once

#include <iostream>
#include <string>

namespace Example
{
    inline void SayHello()
    {
        std::cout << "Hello!\n";
    }

    inline int Square(int x)
    {
        return x * x;
    }

    class Animal
    {
    public:
        std::string name;
        int age = 0;

        Animal(std::string name, int age) : name(std::move(name)), age(age) {}

        virtual std::string Speak() const
        {
            return name + " makes a sound.";
        }

        virtual ~Animal() = default;
    };

    class Dog : public Animal
    {
    public:
        std::string breed;

        Dog(std::string name, int age, std::string breed)
            : Animal(std::move(name), age), breed(std::move(breed)) {}

        std::string Speak() const override
        {
            return name + " barks!";
        }

        void Fetch(const std::string& item) const
        {
            std::cout << name << " fetches the " << item << "!\n";
        }
    };

    class Cat : public Animal
    {
    public:
        bool indoor = true;

        Cat(std::string name, int age, bool indoor = true)
            : Animal(std::move(name), age), indoor(indoor) {}

        std::string Speak() const override
        {
            return name + " meows!";
        }

        void Purr() const
        {
            std::cout << name << " purrs...\n";
        }
    };

    inline std::string MakeAnimalSpeak(const Animal& animal)
    {
        return animal.Speak();
    }

    class Vector3
    {
    public:
        float x = 0, y = 0, z = 0;

        Vector3() = default;
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

        Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
        Vector3 operator*(float s)          const { return {x * s,   y * s,   z * s};   }
        Vector3 operator/(float s)          const { return {x / s,   y / s,   z / s};   }

        Vector3& operator+=(const Vector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
        Vector3& operator-=(const Vector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
        Vector3& operator*=(float s)          { x *= s;   y *= s;   z *= s;   return *this; }
        Vector3& operator/=(float s)          { x /= s;   y /= s;   z /= s;   return *this; }

        bool operator==(const Vector3& o) const { return x == o.x && y == o.y && z == o.z; }
        bool operator!=(const Vector3& o) const { return !(*this == o); }

        float Dot(const Vector3& o)   const { return x*o.x + y*o.y + z*o.z; }
        Vector3 Cross(const Vector3& o) const
        {
            return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x};
        }
        float LengthSquared() const { return x*x + y*y + z*z; }

        std::string ToString() const
        {
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
        }
    };

    inline Vector3 operator*(float s, const Vector3& v) { return v * s; }
}
