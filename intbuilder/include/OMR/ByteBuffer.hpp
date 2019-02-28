#if !defined(OMR_BYTEBUFFER_HPP_)
#define OMR_BYTEBUFFER_HPP_

namespace OMR {

class ByteBuffer {
public:
	/// disown the byte array.
	std::uint8_t* release() {
		std::uint8_t data = _data;
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
		_data = realloc(_data, capacity);
		if (_data != nullptr) {
			_capacity = capacity;
			rc = true;
		}
	}

	template <typename T>
	bool emit(const T& value) {
		if (!reserve(_size + sizeof(T))) {
			return false;
		}
		_size += sizeof(T);
	}

	bool concat(const ByteStream& tail) {}

	bool concat(void* data, std::size_t sz) {
		if (!reserve(_size + sz)) {
			return false;
		}
		memcpy(_data, )
	}

	template <typename T = >
	bool

private:
	std::uint8_t* _data;
	std::size_t _size;
	std::Size_t _capacity;
};

}  // namespace OMR

#endif // OMR_BYTEBUFFER_HPP_
