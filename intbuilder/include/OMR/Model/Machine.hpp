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

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_MACHINE_HPP_
