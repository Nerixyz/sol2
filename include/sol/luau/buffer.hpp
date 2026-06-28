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

#ifndef SOL_LUAU_BUFFER_HPP
#define SOL_LUAU_BUFFER_HPP

#include <sol/version.hpp>

#if SOL_IS_ON(SOL_USE_LUAU)

#include <string_view>

namespace sol::luau {

	struct [[nodiscard]] SOL_GSL_POINTER buffer_view {

		using value_type = void;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		constexpr buffer_view() = default;
		constexpr buffer_view(SOL_CLANG_LIFETIME_BOUND void* data, size_type size) : data_(data), size_(size) {
		}
		constexpr buffer_view(SOL_CLANG_LIFETIME_BOUND void* begin, void* end) : data_(begin), size_(static_cast<char*>(end) - static_cast<char*>(begin)) {
		}

		template <typename Container,
		          typename = std::enable_if_t<std::conjunction_v<std::is_convertible<decltype(std::declval<const Container&>().data()), void*>,
		                                                         std::is_integral<decltype(std::declval<const Container&>().size())>>,
		                                      void>>
		constexpr buffer_view(const Container& c) : data_(c.data()), size_(c.size()) {
		}

		[[nodiscard]]
		constexpr bool empty() const noexcept {
			return size_ == 0;
		}

		[[nodiscard]]
		constexpr pointer data() const noexcept {
			return data_;
		}

		[[nodiscard]]
		constexpr size_type size() const noexcept {
			return size_;
		}

		[[nodiscard]]
		explicit operator std::string_view() const noexcept {
			return string_view();
		}

        [[nodiscard]]
        constexpr std::string_view string_view() const noexcept{
			return { static_cast<const char*>(data_), size_ };
        }

	private:
		void* data_ = nullptr;
		size_type size_ = 0;
	};

	struct [[nodiscard]] SOL_GSL_POINTER const_buffer_view {

		using value_type = void;
		using const_pointer = const value_type*;
		using size_type = size_t;
		using difference_type = ptrdiff_t;

		constexpr const_buffer_view() = default;
		constexpr const_buffer_view(SOL_CLANG_LIFETIME_BOUND const void* data, size_type size) : data_(data), size_(size) {
		}
		constexpr const_buffer_view(SOL_CLANG_LIFETIME_BOUND const void* begin, void* end)
		: data_(begin), size_(static_cast<const char*>(end) - static_cast<const char*>(begin)) {
		}

		template <typename Container,
		          typename = std::enable_if_t<std::conjunction_v<std::is_convertible<decltype(std::declval<const Container&>().data()), const void*>,
		                                                         std::is_integral<decltype(std::declval<const Container&>().size())>>,
		                                      void>>
		constexpr const_buffer_view(const Container& c) : data_(c.data()), size_(c.size()) {
		}

		[[nodiscard]]
		constexpr bool empty() const noexcept {
			return size_ == 0;
		}

		[[nodiscard]]
		constexpr const_pointer data() const noexcept {
			return data_;
		}

		[[nodiscard]]
		constexpr size_type size() const noexcept {
			return size_;
		}

		[[nodiscard]]
		explicit operator std::string_view() const noexcept {
			return string_view();
		}

        [[nodiscard]]
        constexpr std::string_view string_view() const noexcept{
			return { static_cast<const char*>(data_), size_ };
        }

	private:
		const void* data_ = nullptr;
		size_type size_ = 0;
	};

} // namespace sol::luau

#endif

#endif
