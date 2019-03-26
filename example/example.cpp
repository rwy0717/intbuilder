#include <stdint.h> // HACK
#include <stddef.h> // HACK

//////////////////////////////////

#include <OMR/BytecodeInterpreterBuilder.hpp>
#include <OMR/BytecodeMethodBuilder.hpp>

#include <OMR/ByteBuffer.hpp>

#include <VirtualMachineState.hpp>
#include <TypeDictionary.hpp>
#include <JitBuilder.hpp>

#include "JitHelpers.hpp"
#include "JitTypes.hpp"
#include "Interpreter.hpp"
#include "Model.hpp"

#include <cstdint>
#include <cassert>
#include <memory>

#define INT_ENABLED
#define JIT_ENABLED

void gen_dbg_msg(JB::IlBuilder* b, const char* file, std::size_t line, const char* func, const char* msg) {
	b->Call("dbg_msg", 4,
		OMR::Model::constant(b, file),
		OMR::Model::constant(b, line),
		OMR::Model::constant(b, func),
		OMR::Model::constant(b, msg)
	);
}

#define GEN_DBG_MSG(b, msg) gen_dbg_msg(b, __FILE__, __LINE__, __FUNCTION__, msg)

#define GEN_TRACE_MSG(b, msg) GEN_DBG_MSG(b, msg)

#define GEN_TRACE(b) GEN_DBG_MSG(b, "trace")

template <OMR::Model::Mode M>
struct GenDispatchValue;

#ifdef INT_ENABLED
template <>
struct GenDispatchValue<OMR::Model::Mode::REAL> {
	OMR::Model::RUIntPtr operator()(JB::IlBuilder* b, Model::RealMachine& machine) {
		return OMR::Model::RUIntPtr::pack(
			b->LoadAt(b->typeDictionary()->pInt8,
				machine.instruction.address(b).unpack()));
	}
};
#endif // INT_ENABLED

template <>
struct GenDispatchValue<Model::Mode::VIRT> {
	Model::CUIntPtr operator()(Model::CBuilder* b, Model::VirtMachine& machine) {
		// translate pc to bytecode index.
		return OMR::Model::CUIntPtr::pack(
			static_cast<std::uintptr_t>(
				*reinterpret_cast<std::uint8_t*>(machine.instruction.address(b).unpack())));
	}
};

template <OMR::Model::Mode M>
struct GenFunctionEntry;

#ifdef INT_ENABLED
template <>
struct GenFunctionEntry<OMR::Model::Mode::REAL> {
	void operator()(JB::IlBuilder* b, Model::RealMachine& machine) {
		OMR_TRACE();
		/// RWY TODO: Initializing the machine this late in the 
		/// method / interpreter is too late. The Func model has to be available
		/// at initialization time, so we can establish the compiled method's parameters.

		GEN_TRACE_MSG(b, "ENTER METHOD");

		JB::IlValue* interpreter = b->Load("interpreter");
		JB::IlValue* target = b->Load("target");

		assert(0); // need to figure out how to initialize the machine
#if 0
		Model::RealMachine::Factory factory;
		factory.setInterpreter(interpreter);
		factory.setFunction(OMR::Model::RUIntPtr::pack(target));
		_machine = std::shared_ptr<Model::Machine<M>>(factory.create(b));
		_machine->commit(b);
#endif

		GEN_TRACE_MSG(b, "MACHINE INITIALIZED");
		b->Call("interp_trace", 2, interpreter, target);
		// _machine->initialize(b);
	}
};
#endif // INT_ENABLED

template <>
struct GenFunctionEntry<OMR::Model::Mode::VIRT> {
	void operator()(OMR::Model::VirtIlBuilder* b, Model::VirtMachine& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "FUNCTION ENTRY");
	}
};

template <OMR::Model::Mode M>
struct GenDefault {
	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "DEFAULT HANDLER");
		halt(b, machine);
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenError {
	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "ERROR (UNKNOWN BYTECODE)");
		halt(b, machine);
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenHalt {
	static constexpr std::size_t INSTR_SIZE = 1;

	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		GEN_TRACE_MSG(b, "HALT");
		halt(b, machine);
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenNop {
	static constexpr std::size_t INSTR_SIZE = 1;

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "NOP");
		next(b, machine, OMR::Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenPushConst {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_CONST_OFFSET = 1;

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "PUSH_CONST");

		JB::IlValue* address = machine.instruction.address(b).toIl(b);
		JB::IlValue* index   = machine.instruction.index(b).toIl(b);

		b->Call("print_s", 1, b->Const((void*)"$$$ PUSH_CONST: pc="));
		b->Call("print_x", 1, address);
		b->Call("print_s", 1, b->Const((void*)" index="));
		b->Call("print_u", 1, index);
		b->Call("print_s", 1, b->Const((void*)"\n"));

		OMR::Model::Int64<M> c = machine.instruction.immediateInt64(b, {b, INSTR_CONST_OFFSET});

		b->Call("print_s", 1, b->Const((void*)"$$$ PUSH_CONST: const-value="));
		b->Call("print_u", 1, c.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		machine.stack.pushInt64(b, c.toIl(b));

		next(b, machine, OMR::Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenAdd {
	static constexpr std::size_t INSTR_SIZE = 1;

	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "ADD");
		JB::IlValue* rhs = machine.stack.popInt64(b);
		JB::IlValue* lhs = machine.stack.popInt64(b);
		machine.stack.pushInt64(b, b->Add(lhs, rhs));
		next(b, machine, OMR::Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenPushLocal {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "PUSH_LOCAL");
		OMR::Model::Size<M> index = machine.instruction.immediateSize(b, {b, INSTR_INDEX_OFFSET});
		JB::IlValue* value = machine.locals.get(b, index);
		machine.stack.pushInt64(b, value);
		next(b, machine, OMR::Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenPopLocal {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "POP_LOCAL");
		OMR::Model::Size<M> index = machine.instruction.immediateSize(b, {b, INSTR_INDEX_OFFSET});
		machine.locals.set(b, index, machine.stack.popInt64(b));
		next(b, machine, OMR::Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenBranchIf {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_TARGET_OFFSET = 1;

	bool operator()(OMR::Model::IlBuilder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "BRANCH_IF");

		auto addr = machine.instruction.address(b);

		b->Call("print_s", 1, b->Const((void*)"$$$ BRANCH_IF: pc="));
		b->Call("print_x", 1, addr.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		OMR::Model::Int64<M> immediate = machine.instruction.immediateInt64(b, {b, INSTR_TARGET_OFFSET});
		OMR::Model::Int64<M> offset = OMR::Model::add(b, immediate, OMR::Model::Int64<M>(b, INSTR_SIZE));
		JB::IlValue* cond = machine.stack.popInt64(b);

		ifCmpNotEqualZero(b, machine, cond, offset);
		b->Call("print_s", 1, b->Const((void*)"$$$ FALSE TAKEN !!!\n"));
		next(b, machine, {b, INSTR_SIZE});
		return true;
	}
};

struct CallBuilderBase {
protected:
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t TARGET_OFFSET = 1;
};

template <Model::Mode>
struct CallBuilder;

template <>
struct CallBuilder<Model::Mode::VIRT> : CallBuilderBase {
	static void build(Model::Machine<Model::Mode::VIRT>& machine, JB::IlBuilder* b) {
		// TODO! machine.pc.next(b, Model::CSize(b, INSTR_SIZE));
	}
};

#ifdef INT_ENABLED
template <>
struct CallBuilder<Model::Mode::REAL> : CallBuilderBase {
	void build(Model::Machine<Model::Mode::REAL>& machine, JB::IlBuilder* b) {
		// TODO! something
	}
};
#endif // INT_ENABLED

#ifdef INT_ENABLED

class InterpreterCompiler {
public:
	static constexpr Model::Mode M = Model::Mode::REAL;

	using HandlerTable = JB::BytecodeInterpreterBuilder::HandlerTable<Model::Machine<M>>;

	InterpreterCompiler() {
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

	HandlerTable* handlers() { return &_handlers; }

	const HandlerTable* handlers() const { return &_handlers; }

	JB::TypeDictionary* typedict() { return &_typedict; }

	const JB::TypeDictionary* typedict() const { return &_typedict; }

private:
	template <typename HandlerT>
	void set(Op op, const HandlerT& handler) {
		_handlers.set(std::uint32_t(op), handler);
	}

	JB::TypeDictionary _typedict;
	HandlerTable _handlers;
};

class BytecodeInterpreterBuilder final : public JB::BytecodeInterpreterBuilder {
public:
	static constexpr Model::Mode M = Model::Mode::REAL;

	BytecodeInterpreterBuilder(InterpreterCompiler* compiler)
		: JB::BytecodeInterpreterBuilder(compiler->typedict(), compiler->handlers()) {
		OMR_TRACE();
		JB::TypeDictionary* t = typeDictionary();
		JitHelpers::define(this);
		DefineParameter("interpreter", t->PointerTo(t->LookupStruct("Interpreter")));
		DefineParameter("target",      t->PointerTo(t->LookupStruct("Func")));
		DefineReturnType(t->NoType);
	}

	virtual JB::IlValue* getOpcode(JB::IlBuilder* b) override {
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

	virtual bool buildIL() override final {
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

private:
	std::unique_ptr<Model::Machine<Model::Mode::REAL>> _machine;
};

#endif // INT_ENABLED

#ifdef JIT_ENABLED

class MethodCompiler {
public:
	static constexpr Model::Mode M = Model::Mode::VIRT;

	using BytecodeHandlerTable = JB::BytecodeHandlerTable<Model::Machine<M>>;

	MethodCompiler()
		: _typedict() {
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

	JB::TypeDictionary* typedict() { return &_typedict; }

	BytecodeHandlerTable* handlers() { return &_handlers; }

private:

	template <typename HandlerT>
	void set(Op op, const HandlerT& handler) {
		_handlers.set(std::uint32_t(op), handler);
	}

	JB::TypeDictionary _typedict;
	BytecodeHandlerTable _handlers;
};

class BytecodeMethodBuilder : public JB::BytecodeMethodBuilder {
public:
	BytecodeMethodBuilder(MethodCompiler* compiler, Func* func)
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

	virtual std::uint32_t getOpcode(std::size_t index) override final {
		return std::uint32_t(_func->body[index]);
	}

	virtual bool buildIL() override final {

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

private:
	Func* _func;
};

#endif // JIT_ENABLED

#ifdef INT_ENABLED
/// Create the interpret function.
///
InterpretFn buildInterpret() {
	InterpreterCompiler compiler;
	BytecodeInterpreterBuilder builder(&compiler);
	void* interpret = nullptr;
	std::int32_t rc = compileMethodBuilder(&builder, &interpret);
	return (InterpretFn)interpret;
}

void interpret_wrap(Interpreter* interpreter, Func* target, InterpretFn interpret) {
	interpret(interpreter, target);
}
#endif // INT_ENABLED

#ifdef JIT_ENABLED
CompiledFn compile(Func* func) {
	MethodCompiler compiler;
	BytecodeMethodBuilder builder(&compiler, func);
	void* result = nullptr;
	std::int32_t rc = compileMethodBuilder(&builder, &result);
	assert(rc == 0);
	return (CompiledFn)result;
}
#endif // JIT_ENABLED

/////////////////////////// Instruction Set

Func* MakeHaltOnlyFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakeNopThenHaltFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::NOP;
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakeTwoNopsThenHaltFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::NOP;
	buffer << Op::NOP;
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakePushConstFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(42);
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakePushTwoConstsFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakeAddTwoConstsFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::ADD;
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakeAddThreeConstsFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::ADD;
	buffer << Op::PUSH_CONST << std::int64_t(222);
	buffer << Op::ADD;
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakePopThenPushToLocalFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func(1, 0);
	buffer << Op::PUSH_CONST << std::int64_t(999);
	buffer << Op::PUSH_CONST << std::int64_t(123);
	buffer << Op::POP_LOCAL  << std::int64_t(0);
	buffer << Op::PUSH_LOCAL << std::int64_t(0);
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

Func* MakeBranchIfTrueFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func(0, 0);
	buffer << Op::PUSH_CONST << std::int64_t(1);        // 00 + 1 + 8
	buffer << Op::BRANCH_IF  << std::int64_t(12);       // 09 + 1 + 8
	buffer << Op::PUSH_CONST << std::int64_t(0x7);      // 18 + 1 + 8
	buffer << Op::HALT;                                 // 27 + 1
	buffer << Op::HALT;                                 // 28 + 1
	buffer << Op::HALT;                                 // 29 + 1
	buffer << Op::PUSH_CONST << std::int64_t(0x8);      // 30 + 1 + 8
	buffer << Op::HALT;                                 // 39 + 1
	return (Func*)buffer.release();
}

extern "C" int main(int argc, char** argv) {
	initializeJit();
	Func* target = MakeBranchIfTrueFunction();
	Interpreter interpreter;

	fprintf(stderr, "@@@ int main: interpreter=%p\n", &interpreter);
	fprintf(stderr, "@@@ int main: target=%p\n", target);
	fprintf(stderr, "@@@ int main: target startpc=%p\n", &target->body[0]);
	fprintf(stderr, "@@@ int main: initial op=%hhu\n", target->body[0]);
	fprintf(stderr, "@@@ int main: initial sp=%p\n", interpreter.sp());

#if defined(JIT_ENABLED) && 0
	{
		fprintf(stderr, "!!! int main: compiling function\n");
		CompiledFn cfunc = compile(target);
		if (cfunc) {
			fprintf(stderr, "!!! int main: running compiled function\n");
			cfunc(&interpreter);
			fprintf(stderr, "!!! end of compiled function\n");
			fprintf(stderr, "!!! int main: stack[0]=%llu\n", interpreter.peek(0));
		}
	}
#endif

#ifdef INT_ENABLED
	{
		fprintf(stderr, "!!! int main: compiling interpreter\n");
		InterpretFn interpret = buildInterpret();
		if (interpret) {
			fprintf(stderr, "!!! int main: running interpreted function\n");
			interpret_wrap(&interpreter, target, interpret);
			fprintf(stderr, "!!! end of interpreted function\n");
		}
	}
#endif

	free(target);
	return 0;
}
