#if !defined(OMR_MODEL_COMPILERBUILDER_HPP_)
#define OMR_MODEL_COMPILERBUILDER_HPP_

#include <BytecodeBuilderTable.hpp>
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

	HandlerMap& handlers() { return _handlers; }

	const HandlerMap& handlers() const { return _handlers; }

	template <typename GenerateT>
	void create(std::uint32_t opcode, GenerateT&& generate) {

	}

private:
	SpecT _spec;
	HandlerMap _handlers;
	GeneratorFn _unknownHandler;
	JB::TypeDictionary* _typedict;
};

class MethodBuilderData {
public:
	JB::BytecodeBuilderTable& bcbuilders() { return _bcbuilders; }

private:
	JB::BytecodeBuilderTable _bcbuilders;
};

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class BytecodeMethodBuilder : public JB::MethodBuilder {
public:
	using Compiler = Compiler<SpecT, MachineT>;

	SpecMethodBuilder(Compiler& compiler)
		: JB::MethodBuilder(compiler.typedict()), _compiler(compiler) {
	}

	virtual bool buildIL() override {
		OMR_TRACE();
		_machine = initialize();
		setVMState(_machine.get());
		AppendBuilder(data()->bcbuilders().get(this, 0));

		std::int32_t index;
		while((index = GetNextBytecodeFromWorklist()) != -1) {

			std::uint8_t* pc = reinterpret_cast<std::uint8_t*>(_func->body + index);
			std::uint8_t op = *pc;
		
			fprintf(stderr, "compiling index=%u opcode=%u\n", index, op);
			OMR_TRACE();
			handlers()[op](data()->bcbuilders().get(this, index), *_machine);
		}

		Return();
		return true;
	}

	virtual std::shared_ptr<MachineT> initialize() = 0;

	typename Compiler::HandlerMap& handlers() const { return _compiler.handlers(); }

	Compiler& compiler() const { return _compiler; }

	MethodBuilderData* data() { return &_data; }

private:
	Compiler& _compiler;
	MethodBuilderData _data;
	std::function
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_COMPILERBUILDER_HPP_
