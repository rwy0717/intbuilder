
#if !defined(OMR_MODEL_FUNCTIONDATA_HPP_)
#define OMR_MODEL_FUNCTIONDATA_HPP_

#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Value.hpp>

#include <BytecodeBuilderTable.hpp>

#include <cstdint>

namespace OMR {
namespace Model {

/// Program metadata interface.
/// Internal API. Must be threaded through to certain vm state helpers.
template <Mode>
class FunctionData;

template <>
class FunctionData<Mode::REAL> {
public:
	explicit FunctionData(RPtr<std::uint8_t> start) : _start(start.unpack()) {}

	JB::IlValue* start() const { return _start; }

private:
	JB::IlValue* _start;
};

template <>
class FunctionData<Mode::VIRT> {
public:
	FunctionData(CPtr<std::uint8_t> start, JB::BytecodeBuilderTable* builders)
		: _start(start.unpack()), _builders(builders) {}

	std::uint8_t* start() const { return _start; }

	JB::BytecodeBuilderTable* builders() const { return _builders; }

private:
	std::uint8_t* _start;
	JB::BytecodeBuilderTable* _builders;
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_FUNCTIONDATA_HPP_
