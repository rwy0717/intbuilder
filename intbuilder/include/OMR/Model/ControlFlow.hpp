#if !defined(OMR_MODEL_CONTROLFLOW_HPP_)
#define OMR_MODEL_CONTROLFLOW_HPP_

#include <OMR/Model/Mode.hpp>

#include <BytecodeBuilder.hpp>

#include <BytecodeBuilderTable.hpp>

namespace OMR {
namespace Model {

template <Mode M>
class ControlFlow;

template <>
class ControlFlow<Mode::REAL> {
public:
	ControlFlow() {}

	void next(OMR_UNUSED JB::IlBuilder* b, OMR_UNUSED std::size_t index) {}
};

template <>
class ControlFlow<Mode::VIRT> {
public:
	ControlFlow(JB::BytecodeBuilderTable* builders)  : _builders(builders) {}

	void next(JB::BytecodeBuilder* b, std::size_t index) {
		b->AddFallThroughBuilder(_builders->get(b, index));
	}

private:
	JB::BytecodeBuilderTable* _builders;
};

using VirtControlFlow = ControlFlow<Mode::VIRT>;
using RealControlFlow = ControlFlow<Mode::REAL>;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_CONTROLFLOW_HPP_
