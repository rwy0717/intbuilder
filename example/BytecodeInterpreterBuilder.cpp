#include <BytecodeInterpreterBuilder.hpp>
#include <JitTypes.hpp>
#include <JitHelpers.hpp>

BytecodeInterpreterCompiler::BytecodeInterpreterCompiler() {
	JitTypes::define(&_typedict);
	set(Op::UNKNOWN,    GenError<M>());
	set(Op::NOP,        GenNop<M>());
	set(Op::HALT,       GenHalt<M>());
	set(Op::PUSH_CONST, GenPushConst<M>());
	set(Op::ADD,        GenAdd<M>());
	set(Op::PUSH_LOCAL, GenPushLocal<M>());
	set(Op::POP_LOCAL,  GenPopLocal<M>());
	set(Op::BRANCH_IF,  GenBranchIf<M>());

	_handlers.setDefault(GenDefault<M>());
}

BytecodeInterpreterBuilder::BytecodeInterpreterBuilder(BytecodeInterpreterCompiler* compiler)
	: JB::BytecodeInterpreterBuilder(compiler->typedict(), compiler->handlers()) {
	OMR_TRACE();
	JB::TypeDictionary* t = typeDictionary();
	JitHelpers::define(this);
	DefineParameter("interpreter", t->PointerTo(t->LookupStruct("Interpreter")));
	DefineParameter("target",      t->PointerTo(t->LookupStruct("Func")));
	DefineReturnType(t->NoType);
}

JB::IlValue* BytecodeInterpreterBuilder::getOpcode(JB::IlBuilder* b) {
	JB::TypeDictionary* t = b->typeDictionary();

	b->Call("print_s", 1, b->Const((void*)"$$$ DISPATCHING\n"));

	JB::IlValue* target = GenDispatchValue<Model::Mode::REAL>()(b, *_machine).unpack();
	JB::IlValue* target32 = b->ConvertTo(t->Int32, target);

	b->Call("interp_trace", 2, b->Load("interpreter"), b->Load("target"));
	b->Call("print_s", 1, b->Const((char*)"$$$ NEXT: next-bc="));
	b->Call("print_x", 1, target);
	b->Call("print_s", 1, b->Const((void*)"\n$$$ NEXT: dispatch: converted-next-bc="));
	b->Call("print_x", 1, target32);
	b->Call("print_s", 1, b->Const((void*)"\n"));

	return target32;
}

bool BytecodeInterpreterBuilder::buildIL() {
	GEN_TRACE_MSG(this, "ENTER METHOD");

	JB::IlValue* interpreter = Load("interpreter");
	JB::IlValue* target = Load("target");

	Model::Machine<M>::Factory factory;
	factory.setInterpreter(interpreter);
	factory.setFunction(Model::RPtr<Func>::pack(target));

	OMR::Model::FunctionData<OMR::Model::Mode::REAL> data(
		OMR::Model::RPtr<std::uint8_t>::pack(
			StructFieldInstanceAddress("Func", "body", target)));

	_machine.reset(factory.create(this, data));
	_machine->commit(this);

	GEN_TRACE_MSG(this, "$$$ MACHINE INITIALIZED");
	Call("interp_trace", 2, interpreter, target);

	bool success = buildInterpreterIL(_machine.get()); // dispatch to superclass

	GEN_TRACE_MSG(this, "$$$ EXIT METHOD");
	Return();
	return success;
}
