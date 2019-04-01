#include "BytecodeMethodBuilder.hpp"
#include "BytecodeHandlers.hpp"

BytecodeMethodCompiler::BytecodeMethodCompiler() : _typedict() {
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

BytecodeMethodBuilder::BytecodeMethodBuilder(BytecodeMethodCompiler* compiler, Func* func)
		: JB::BytecodeMethodBuilder(compiler->typedict(), compiler->handlers())
		, _func(func) {

		DefineName("compiled-method");
		DefineLine("0");
		DefineFile("<generated>");

		OMR_TRACE();

		JB::TypeDictionary* t = this->typeDictionary();
		JitHelpers::define(this);
		DefineParameter("interpreter", t->PointerTo(t->LookupStruct("Interpreter")));
		DefineReturnType(t->NoType);
	}

std::uint32_t BytecodeMethodBuilder::getOpcode(std::size_t index) {
	return std::uint32_t(_func->body[index]);
}

bool BytecodeMethodBuilder::buildIL() {
	OMR_TRACE();

	Model::VirtMachine::Factory factory;
	factory.setInterpreter(Load("interpreter"));
	factory.setFunction(Model::CPtr<Func>::pack(_func));

	OMR::Model::FunctionData<Model::Mode::VIRT> data(OMR::Model::CPtr<std::uint8_t>::pack(_func->body), builders());
	std::shared_ptr<Model::VirtMachine> machine(factory.create(this, data));
	setVMState(machine.get());

	buildBytecodeIL(/* machine.get(), _func */);

	Return();
	return true;
}
