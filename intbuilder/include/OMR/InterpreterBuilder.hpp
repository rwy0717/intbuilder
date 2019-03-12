#if !defined(OMR_MODEL_INTERPRETERBUILDER_HPP_)
#define OMR_MODEL_INTERPRETERBUILDER_HPP_

#include <MethodBuilder.hpp>

#include <cstdint>
#include <cstddef>

namespace OMR {
namespace Model {

namespace JB = ::OMR::JitBuilder;

template <typename SpecT, typename MachineT = typename SpecT::MachineType>
class InterpreterBuilder : public JB::MethodBuilder {
public:
	class HandlerTable {
	public:
		HandlerTable(InterpreterBuilder* ib) :
			_ib(ib) {}

		template <typename GenerateT>
		void create(std::uint32_t opcode, GenerateT&& generate) {
			JB::IlBuilder* b = nullptr;
			_cases.push_back(_ib->MakeCase(opcode, &b, false));
			MachineT machine = _ib->spec().machine();
			machine.reload(b);
			generate(b, machine);
		}

		std::vector<JB::IlBuilder::JBCase*>& cases() { return _cases; }

		const std::vector<JB::IlBuilder::JBCase*>& cases() const { return _cases; }

	private:
		InterpreterBuilder* _ib;
		std::vector<JB::IlBuilder::JBCase*> _cases;
	};

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

		JB::IlBuilder* loop = OrphanBuilder();
		DoWhileLoop((char*)"interpreter_continue", &loop);

		loop->Store("iterpreter_opcode", _spec.genDispatchValue(loop, _spec.machine()));

#if 1
		loop->Call("print_s", 1, loop->Const((void*)"*** WHILE LOOP START: opcode="));
		loop->Call("print_u", 1, loop->Load("interpreter_opcode"));
		loop->Call("print_s", 1, loop->Const((void*)"\n"));
#endif

		MachineT machine = _spec.machine();
		JB::IlBuilder* defaultHandler = genDefaultHandler();
		std::vector<JB::IlBuilder::JBCase*> handlers = genHandlers();
		loop->Switch("interpreter_opcode", &defaultHandler, handlers.size(), handlers.data());
		
		_spec.genLeaveMethod(this);
		Return();
		return true;
	}

	JB::IlBuilder* genDefaultHandler() {
		JB::IlBuilder* b = OrphanBuilder();
		MachineT machine = _spec.machine();
		machine.reload(b);
		_spec.genDefaultHandler(b, machine);
		return b;
	}

	std::vector<JB::IlBuilder::JBCase*> genHandlers() {
		HandlerTable table(this);
		_spec.genHandlers(table);
		return table.cases();
	}

	SpecT& spec() { return _spec; }

	const SpecT& spec() const { return _spec; }

private:
	SpecT _spec;
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_INTERPRETERBUILDER_HPP_
