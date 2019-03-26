#if !defined(OMR_MODEL_MACHINE_HPP_)
#define OMR_MODEL_MACHINE_HPP_

#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Pc.hpp>

#include <VirtualMachineState.hpp>

namespace OMR {
namespace Model {

template <Mode M>
class Machine : public JitBuilder::VirtualMachineState {
public:
	Machine(BytecodeBuilderTable* builders) {}

	ControlFlow<M>* controlFlow() { return _controlFlow; }

	const ControlFlow<M>* controlFlow() const { return _controlFlow; }

private:
	ControlFlow<M> _controlFlow;
};

#if 0
// TODO RWY: Implement generic instruction/machine code
template <Mode M>
class Instruction;

template <>
class Instruction<Mode::REAL> {
public:
	RSize offset() {}

	RUIntPtr address() {}
};

template <>
class Instruction<Model::VIRT> {
	CSize offset() {}

	CUIntPtr address() {}
};
#endif ///


}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_MACHINE_HPP_
