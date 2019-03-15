#if !defined(OMR_MODEL_CONTROLFLOW_HPP_)
#define OMR_MODEL_CONTROLFLOW_HPP_

#include <OMR/Model/Mode.hpp>
#include <OMR/SpecMethodBuilder.hpp>

#include <BytecodeBuilder.hpp>

namespace OMR {
namespace Model {

template <Mode M>
class ControlFlow;

template <>
class ControlFlow<Mode::REAL> {
public:
	ControlFlow() {}
};

template <>
class ControlFlow<Mode::VIRT> {
public:
	ControlFlow(MethodBuilderData* data)  :_data(data) {}

	void next(JB::IlBuilder* b, std::size_t target) {
		JB::BytecodeBuilder* bb = static_cast<JB::BytecodeBuilder*>(b);
		bb->AddFallThroughBuilder(_data->bcbuilders().get(bb, target));
	}

private:
	MethodBuilderData* _data;
};

using VirtControlFlow = ControlFlow<Mode::VIRT>;
using RealControlFlow = ControlFlow<Mode::REAL>;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_CONTROLFLOW_HPP_
