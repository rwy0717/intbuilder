#if !defined(OMR_BYTEBUFFER_HPP_)
#define OMR_BYTEBUFFER_HPP_

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <cstdio>

namespace OMR {

typedef std::uint8_t Byte;

class ByteBuffer {
public:
	ByteBuffer() : _data(nullptr), _size(0), _capacity(0) {}

	ByteBuffer(std::size_t capacity) : ByteBuffer() { resize(capacity); }

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

	void clear() {
		_capacity = 0;
		if (_data != nullptr) {
			std::free(_data);
			_data = nullptr;
		}
	}

	bool resize(std::size_t capacity) {
		if (capacity == 0) {
			clear();
			return true;
		}
		_data = (std::uint8_t*)std::realloc(_data, capacity);
		if (_data != nullptr) {
			_capacity = capacity;
			return true;
		}
		_capacity = 0;
		return false;
	}

	bool reserve(std::size_t capacity) {
		if (capacity <= _capacity) {
			return true;
		}
		return resize(capacity);
	}

	/// Grow to hold a minimum of min-capacity.
	/// Growth occurs by doubling.
	bool grow(std::size_t minCapacity) {
		std::size_t newCapacity = _capacity == 0 ? BASE_CAPACITY : _capacity; // std::max(_capacity, BASE_CAPACITY);
		while (newCapacity < minCapacity) {
			newCapacity *= GROW_FACTOR;
		}
		return reserve(newCapacity);
	}

	template <typename T>
	bool append(const T& value) {
		if (!grow(_size + sizeof(T))) {
			return false;
		}
		std::memcpy(end(), (void*)&value, sizeof(T));
#if 1
		fprintf(stderr, "emit: nbytes=%zu\n", sizeof(T));
		for (std::size_t i = 0; i < sizeof(T); ++i) {
			fprintf(stderr, " emit: [%zu:%p]=%hhu\n", i, end() + i, end()[i]);
		}
#endif //

		_size += sizeof(T);
		return true;
	}

	std::uint8_t* data() { return _data; }

	const std::uint8_t* data() const { return _data; }

	std::uint8_t* start() { return _data; }

	const std::uint8_t* start() const { return _data; }

	std::uint8_t* end() { return _data + _size; }

	const std::uint8_t* end() const { return _data + _size; }

	const std::uint8_t* cstart() const { return _data; }

	const std::uint8_t* cend() const { return _data + _size; }

	/// Read a value from the byte stream.
	std::uint8_t& at(std::size_t offset) { return _data[offset]; }

	const std::uint8_t& at(std::size_t offset) const { return _data[offset]; }

private:
	static constexpr std::size_t BASE_CAPACITY = 64;
	static constexpr std::size_t GROW_FACTOR = 2;

	std::uint8_t* _data = nullptr;
	std::size_t _size = 0;
	std::size_t _capacity = 0;
};

template <typename T>
ByteBuffer& operator<<(ByteBuffer& buffer, const T& value) {
	buffer.append(value);
	return buffer;
}

}  // namespace OMR

#endif // OMR_BYTEBUFFER_HPP_
