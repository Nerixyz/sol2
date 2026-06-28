// sol2

// The MIT License (MIT)

// Copyright (c) 2013-2026 Rapptz, ThePhD and contributors

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_LUAU_VECTOR_HPP
#define SOL_LUAU_VECTOR_HPP

#include <sol/version.hpp>

#if SOL_IS_ON(SOL_USE_LUAU)

#include <sol/compatibility/lua_version.hpp>
#include <array>

namespace sol::luau {

	struct [[nodiscard]] vector {
#if LUA_VECTOR_SIZE == 3
		constexpr vector() noexcept : components({ 0, 0, 0 }) {
		}
		constexpr vector(float x, float y, float z) noexcept : components({ x, y, z }) {
		}
		constexpr static vector from_pointer(const float* p) noexcept {
			return vector(p[0], p[1], p[2]);
		}
#elif LUA_VECTOR_SIZE == 4
		constexpr vector() noexcept : components({ 0, 0, 0, 0 }) {
		}
		constexpr vector(float x, float y, float z, float w) noexcept : components({ x, y, z, w }) {
		}
		constexpr static vector from_pointer(const float* p) noexcept {
			return vector(p[0], p[1], p[2], p[3]);
		}
#else
#error Unsupported vector size
#endif

		constexpr vector(std::array<float, LUA_VECTOR_SIZE> components) noexcept : components(components) {
		}

		[[nodiscard]]
		constexpr float x() const noexcept {
			return components[0];
		}

		[[nodiscard]]
		constexpr float y() const noexcept {
			return components[1];
		}

		[[nodiscard]]
		constexpr float z() const noexcept {
			return components[2];
		}

#if LUA_VECTOR_SIZE == 4
		[[nodiscard]]
		constexpr float w() const noexcept {
			return components[3];
		}
#endif

		constexpr void set_x(float x) noexcept {
			components[0] = x;
		}

		constexpr void set_y(float y) noexcept {
			components[1] = y;
		}

		constexpr void set_z(float z) noexcept {
			components[2] = z;
		}

#if LUA_VECTOR_SIZE == 4
		constexpr void set_w(float w) noexcept {
			components[3] = w;
		}
#endif

		constexpr vector with_x(float x) const noexcept {
			vector v(*this);
			v.set_x(x);
			return v;
		}

		constexpr vector with_y(float y) const noexcept {
			vector v(*this);
			v.set_y(y);
			return v;
		}

		constexpr vector with_z(float z) const noexcept {
			vector v(*this);
			v.set_z(z);
			return v;
		}

#if LUA_VECTOR_SIZE == 4
		constexpr vector with_w(float w) const noexcept {
			vector v(*this);
			v.set_w(w);
			return v;
		}
#endif

#if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L
		constexpr auto operator<=>(const vector& rhs) const noexcept = default;
#else
		bool operator==(const vector& rhs) const noexcept {
			return components == rhs.components;
		}
		bool operator!=(const vector& rhs) const noexcept {
			return components != rhs.components;
		}
		bool operator<(const vector& rhs) const noexcept {
			return components < rhs.components;
		}
		bool operator<=(const vector& rhs) const noexcept {
			return components <= rhs.components;
		}
		bool operator>(const vector& rhs) const noexcept {
			return components >= rhs.components;
		}
		bool operator>=(const vector& rhs) const noexcept {
			return components >= rhs.components;
		}
#endif

		std::array<float, LUA_VECTOR_SIZE> components;
	};

} // namespace sol::luau

#endif

#endif
