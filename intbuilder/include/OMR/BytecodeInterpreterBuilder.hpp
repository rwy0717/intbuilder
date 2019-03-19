#if !defined(OMR_JITBUILDER_BYTECODEINTERPRETERBUILDER_HPP_)
#define OMR_JITBUILDER_BYTECODEINTERPRETERBUILDER_HPP_

#include <MethodBuilder.hpp>

#include <cstdint>
#include <cstddef>

///////////////////// TODO FIX TO REMOVE TEMPLATE PARAMETERS
#if 0

namespace OMR {
namespace JitBuilder {

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class BytecodeInterpreterBuilder;

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class BytecodeHandlerTable {
public:
	BytecodeHandlerTable(BytecodeInterpreterBuilder* bcInterpreterBuilder)
		: _bcInterpreterBuilder(BytecodeInterpreterBuilder) {}

	template <typename GenerateT>
	void create(std::uint32_t opcode, GenerateT&& generate) {
		JB::IlBuilder* b = nullptr;
		_cases.push_back(_ib->MakeCase(opcode, &b, false));
		MachineT machine = _ib->spec().machine();
		machine.reload(b);
		generate(b, machine);
		// machine.mergeInto(_ib.spec().machine());
	}

	std::vector<JB::IlBuilder::JBCase*>& cases() { return _cases; }

	const std::vector<JB::IlBuilder::JBCase*>& cases() const { return _cases; }

private:
	InterpreterBuilder* _ib;
	std::vector<JB::IlBuilder::JBCase*> _cases;
};

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class BytecodeInterpreterBuilder : public MethodBuilder {
public:
	template <typename... Args>
	InterpreterBuilder(JB::TypeDictionary* t, Args&&... args) :
		JB::MethodBuilder(t),
		_spec(std::forward<Args>(args)...) {

		DefineName("interpret");
		DefineLine("0");	
		DefineFile("<generated>");

		DefineLocal("interpreter_opcode", t->Int32);
		DefineLocal("interpreter_continue", t->Int32);

		_spec.initialize(this);

		AllLocalsHaveBeenDefined();
	}

	~InterpreterBuilder() {
		_spec.tearDown(this);
	}

	virtual bool buildIL() override final {

		Store("interpreter_opcode", Const(std::int32_t(-1)));
		Store("interpreter_continue", Const(std::int32_t(1)));

		_spec.genEnterMethod(this);

		/// allocate a clone of the spec's machine.
		_machine.reset(new MachineT(_spec.machine()));

		JB::IlBuilder* loop = OrphanBuilder();
		DoWhileLoop((char*)"interpreter_continue", &loop);

		{
			MachineT machine = _spec.machine();
			machine.reload(loop);
			_spec.genDispatchValue(loop, machine);
			loop->Store("interpreter_opcode", _spec.genDispatchValue(loop, machine));
		}
	
		loop->Call("print_s", 1, loop->Const((void*)"$$$ *** INTERPRETING: opcode="));
		loop->Call("print_u", 1, loop->Load("interpreter_opcode"));
		loop->Call("print_s", 1, loop->Const((void*)"\n"));

		JB::IlBuilder* defaultHandler = genDefaultHandler();
		std::vector<JB::IlBuilder::JBCase*> handlers = genHandlers();

		loop->Switch("interpreter_opcode", &defaultHandler, handlers.size(), handlers.data());

		_spec.genLeaveMethod(this);
	
		Return();

		return true;
	}

	JB::IlBuilder* genDefaultHandler() {
		JB::IlBuilder* b = OrphanBuilder();
		MachineT clone = *_machine;
		clone.reload(b);
		_spec.genDefaultHandler(b, clone);
		return b;
	}

	std::vector<JB::IlBuilder::JBCase*> genHandlers() {
		HandlerTable table(this);
		_spec.genHandlers(table);
		return table.cases();
	}

	SpecT& spec() { return _spec; }

	const SpecT& spec() const { return _spec; }

	MachineT& machine() { return *_machine; }

private:
	SpecT _spec;
	std::unique_ptr<MachineT> _machine = nullptr;
};

}  // namespace JitBuilder
}  // namespace OMR


#endif ///////////////////////////////// 0


#endif // OMR_JITBUILDER_BYTECODEINTERPRETERBUILDER_HPP_
