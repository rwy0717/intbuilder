#if !defined(OMR_BYTECODECOMPILEDRIVER_HPP_)
#define OMR_BYTECODECOMPILEDRIVER_HPP_

#include "ilgen/BytecodeBuilder.hpp"

#include <unordered_map>

template <typename T>
struct Span final {
public:
	Span() = default;

	Span(const Span<T>&) = default;

	Span(T* data, std::size_t length) : _data(data), _length(length) {}

	T* data() const { return _data; }

	std::size_t length() const { return length; }

	std::size_t nbytes() const { return _length * sizeof(T); }

private:
	T* data_;
	std::size_t length_;  // in elements
};

using Byte = std::uint8_t;
using ByteSpan = Span<Byte>;
using ConstByteSpan = Span<const Byte>;

using BytecodeBuilderTable = std::unordered_map<std::size_t, TR::BytecodeBuilder*>;

namespace OMR {

class BytecodeCompileDriver {
public:
	BytecodeCompileDriver()
	BytecodeBuilderTable& table() { return _table; }
	const BytecodeBuilderTable& table() const { return _table; }


	BytecodeBuilder* at(std::size_t index) {}
	std::size_t offset

	const ByteSpan& program() { return _program; }

private:
	Span<const std::uint8_t> _program;
	BytecodeBuilderTable _table;
};

} // namespace OMR

#endif // OMR_BYTECODECOMPILEDRIVER_HPP_
