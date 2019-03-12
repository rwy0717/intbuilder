#if !defined(OMR_MODEL_ILGENERATOR)
#define OMR_MODEL_ILGENERATOR_HPP_

#include <OMR/Model/Mode.hpp>

#include <functional>

namespace OMR {
namespace JitBuilder {

class IlBuilder;

}  // namespace JitBuilder
}  // namespace OMR

namespace OMR {
namespace Model {

/// IL Generation class.
template <Mode M, typename MachineT>
class Generator {
public:
	virtual void generate(JitBuilder::IlBuilder* b, MachineT& machine) = 0;
};

template <Mode M>
class GenerationContext;

template <>
class GenerationContext<Mode::REAL> {
public:
	void generate()
};


using Handler = std::function<void(JB::IlBuilder*)>;

struct HandlerSpec {
	std::uint32_t code;
	Handler handler;
};

struct Spec {
	std::function<void(JB::MethodBuilder*)> initMethod;
	std::function<void(JB::MethodBuilder*)> killMethod;
	std::function<void(JB::MethodBuilder*)> enterMethod;
	std::function<void(JB::MethodBuilder*)> leaveMethod;
	std::vector<HandlerSpec> handlers;
	Handler defaultHandler;
};

template <typename MachineT, typename InstructionSetT>
void* genInterpreter(MachineT& machine, InstructionSetT& instructions) {


	for(auto& instruction : instructions.generators()) {
		MachineT clone = machine;
		clone.reload();
		instruction.generator(clone);
	}
};

}  // namespace Model
}  // namespace OMR

#endif  /// OMR_MODEL_ILGENERATOR_HPP_
