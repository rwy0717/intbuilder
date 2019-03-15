#if !defined(OMR_SPAN_HPP_)
#define OMR_SPAN_HPP_

#include <cstddef>
#include <cassert>

namespace OMR {

/// A range of unowned data.
///
template <typename T>
struct Span final {
public:
	Span() = default;

	Span(const Span<T>&) = default;

	Span(T* data, std::size_t length) : _data(data), _length(length) {}

	T* data() const { return _data; }

	std::size_t length() const { return length; }

	std::size_t nbytes() const { return _length * sizeof(T); }

	T& operator[](std::size_t index) {
		return data_[index];
	}

	T& at(std::size_t index) {
		assert(index < length_);
		return data_[index];
	}

	T* begin() const { return }

	T* end() const { return data_ + length; }

	T* cbegin() const { return begin(); }

	T* cend() const { return cend(); }

private:
	T* data_;
	std::size_t length_;  // in elements
};

}  // namespace OMR

#endif // OMR_SPAN_HPP_
