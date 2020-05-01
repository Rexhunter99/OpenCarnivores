#pragma once


namespace Math {
	//const float pi = 3.14159265358979323846L;
	constexpr float pi = 3.14159265358979323846L;
	constexpr double pi = 3.14159265358979323846L;
	constexpr long double pi = 3.14159265358979323846L;
};


/*
C++ Template for Vector3d
TODO: Replace all usage of Vector3d with vec3<T>
*/
template<typename T> class Vec3 {
public:
	T x, y, z;

	Vec3() {
		x = T();
		y = T();
		z = T();
	}

	Vec3(T x) : x(x), y(x), z(x) {
	}

	Vec3(T x, T y, T z) : x(x), y(y), z(z) {
	}

	Vec3(const Vec3<T>& v) : x(v.x), y(v.y), z(v.z) {
	}

	Vec3<T> operator= (const Vec3<T>& v);
	Vec3<T> operator+ (const Vec3<T>& v);
	Vec3<T> operator- (const Vec3<T>& v);
	Vec3<T> operator* (const Vec3<T>& v);
	Vec3<T> operator/ (const Vec3<T>& v);
	Vec3<T> operator+ (T f);
	Vec3<T> operator- (T f);
	Vec3<T> operator* (T f);
	Vec3<T> operator/ (T f);

	float Length();
	void Normalize();
};

template<typename T> Vec3<T> Normalize(const Vec3<T>& v, T s = 1.0f);
template<typename T> T Dot(Vec3<T> const& a, Vec3<T> const& b);
template<typename T> Vec3<T> Cross(Vec3<T> const& a, Vec3<T> const& b);

/* Pseudo Random generator */
namespace Random {
	void Seed(unsigned seed);
	template<typename T> T Get();
	template<typename T> T Get(T max);
	template<typename T> T Get(T min, T max);
};
