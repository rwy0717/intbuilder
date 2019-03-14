#if !defined(OMR_MODEL_COMPILERBUILDER_HPP_)
#define OMR_MODEL_COMPILERBUILDER_HPP_

#include <MethodBuilder.hpp>
#include <IlBuilder.hpp>

#include <map>
#include <vector>

namespace OMR {
namespace Model {

namespace JB = ::OMR::JitBuilder;

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class Compiler {
public:
	using DispatchFn = std::function<CUInt32(JB::IlBuilder*, MachineT&)>;
	using GeneratorSig = void(JB::IlBuilder*, MachineT&);
	using GeneratorFn = std::function<GeneratorSig>;
	using HandlerMap = std::map<std::uint32_t, GeneratorFn>;

	Compiler(JB::TypeDictionary* typedict) : _typedict(typedict) {

		_spec.genHandlers(*this);
	}

	SpecT& spec() { return _spec; }

	const SpecT& spec() const { return _spec; }

	JB::TypeDictionary* typedict() const { return _typedict; }

	template <typename GenerateT>
	void create(std::uint32_t opcode, GenerateT&& generate) {
		assert(_handlers.count(opcode) == 0); // do not allow double inserts.
		_handlers[opcode] = GeneratorFn(generate);
		fprintf(stderr, "create handler bc=%x handler=%p\n", opcode, _handlers[opcode]. template target<void>());
	}

private:
	SpecT _spec;
	HandlerMap _handlers;
	GeneratorFn _unknownHandler;
	JB::TypeDictionary* _typedict;
};

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class SpecMethodBuilder : public JB::MethodBuilder {
public:
	using Compiler = Compiler<SpecT, MachineT>;

	SpecMethodBuilder(Compiler& compiler)
		: JB::MethodBuilder(compiler.typedict()), _compiler(compiler) {
		_compiler.spec().initialize(this);
	}

	virtual bool buildIL() override final {

		std::shared_ptr<MachineT> machine = initialize();

		return false;
	}

	virtual std::shared_ptr<MachineT> initialize() = 0;

private:
	Compiler& _compiler;
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_COMPILERBUILDER_HPP_
