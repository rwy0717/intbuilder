#if !defined(OMR_BYTECODECOMPILEDRIVER_HPP_)
#define OMR_BYTECODECOMPILEDRIVER_HPP_

#include "ilgen/BytecodeBuilder.hpp"

#include <unordered_map>

namespace OMR {

class BytecodeCompileDriver {
public:
	BytecodeCompileDriver()

	BytecodeBuilderTable& table() { return _table; }

	const BytecodeBuilderTable& table() const { return _table; }

	BytecodeBuilder* at(std::size_t index) {}

	const ByteSpan& program() { return _program; }

private:
	Span<const std::uint8_t> _program;
	BytecodeBuilderTable _table;
};

} // namespace OMR

#endif // OMR_BYTECODECOMPILEDRIVER_HPP_
