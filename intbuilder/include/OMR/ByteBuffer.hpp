#if !defined(OMR_BYTEBUFFER_HPP_)
#define OMR_BYTEBUFFER_HPP_

#include <cstdint>
#include <cstddef>

namespace OMR {

class ByteBuffer {
public:
	/// disown the byte array.
	std::uint8_t* release() {
		std::uint8_t* data = _data;
		_data = nullptr;
		_capacity = 0;
		_size = 0;
		return data;
	}

	std::size_t size() const { return _size; }

	std::size_t capacity() const { return _capacity; }

	bool reserve(std::size_t capacity) {
		if (capacity <= _capacity) {
			return true;
		}
		return resize(capacity);
	}

	void clear() {
		_capacity = 0;
		if (_data != nullptr) {
			free(_data);
			_data = nullptr;
		}
	}

	bool resize(std::size_t capacity) {
		if (capacity == 0) {
			clear();
			return true;
		}
		_data = (std::uint8_t*)realloc(_data, capacity);
		if (_data != nullptr) {
			_capacity = capacity;
			return true;
		}
		return false;
	}

	template <typename T>
	bool emit(const T& value) {
		if (!grow(_size + sizeof(T))) {
			return false;
		}
		memcpy(end() - 1, (void*)&value, sizeof(T));
		_size += sizeof(T);
		return true;
	}

	std::uint8_t* start() { return _data; }

	std::uint8_t* end() { return _data + _size; }

	const std::uint8_t* start() const { return _data; }

	const std::uint8_t* end() const { return _data + _size; }

private:
	bool grow(std::size_t mincapa) {
		std::size_t newcapa = _capacity != 0 ? _capacity : 128;
		while (newcapa < mincapa) {
			newcapa *= 2;
		}
		return reserve(newcapa);
	}

	std::uint8_t* _data = nullptr;
	std::size_t _size = 0;
	std::size_t _capacity = 0;
};

template <typename T>
ByteBuffer& operator<<(ByteBuffer& buffer, const T& value) {
	buffer.emit(value);
	return buffer;
}

}  // namespace OMR

#endif // OMR_BYTEBUFFER_HPP_

//// XXXXXXXXXXXXXXXzzzzzzzzzzzzzzzzz|
//// |--------------|----------------|
//// | start        |                |
////                | end            |
////                                 | capacity
