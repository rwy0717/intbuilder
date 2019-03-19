#include <stdint.h> // HACK
#include <stddef.h> // HACK

//////////////////////////////////

#include <OMR/Model/Value.hpp>
#include <OMR/Model/OperandStack.hpp>
#include <OMR/Model/OperandArray.hpp>
#include <OMR/Model/Register.hpp>
#include <OMR/Model/Pc.hpp>
#include <OMR/Model/Builder.hpp>

#include <OMR/BytecodeInterpreterBuilder.hpp>
#include <OMR/BytecodeMethodBuilder.hpp>

#include <OMR/ByteBuffer.hpp>

#include <VirtualMachineState.hpp>
#include <TypeDictionary.hpp>
#include <JitBuilder.hpp>

#include "JitHelpers.hpp"
#include "JitTypes.hpp"
#include "Interpreter.hpp"

#include <cstdint>
#include <cassert>
#include <memory>

namespace Model = OMR::Model;
namespace JB = OMR::JitBuilder;

/// Wrapper for accessing Func structures through the machine model.
/// Note that this is not the FP in the interpreter, it models the function data itself.
/// In Mode::VIRT, the func is static data used by the compiler.
/// In Mode::REAL, the func is read by the interpreter at run-time.
template <Model::Mode M>
class ModelFunc;

template <>
class ModelFunc<Model::Mode::VIRT> {
public:
	ModelFunc() : _function(nullptr) {}

	void initialize(OMR_UNUSED JB::IlBuilder* b, Model::CUIntPtr function) {
		// TODO: suppport RPtr<Func> rather than the unsafe RUIntPtr.
		_function = reinterpret_cast<Func*>(function.unpack());
	}

	Model::CSize nlocals(OMR_UNUSED JB::IlBuilder* b) {
		return Model::CSize::pack(_function->nlocals);
	}

	Model::CSize nparams(OMR_UNUSED JB::IlBuilder* b) {
		return Model::CSize::pack(_function->nparams);
	}

	Model::CPtr<std::uint8_t> body(OMR_UNUSED JB::IlBuilder* b) {
		return Model::CPtr<std::uint8_t>::pack(
			_function->body
		);
	}

	Model::CValue<CompiledFn> cbody(OMR_UNUSED JB::IlBuilder* b) {
		return Model::CValue<CompiledFn>::pack(_function->cbody);
	}

	Func* unpack() { return _function; }

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, ModelFunc<Model::Mode::VIRT>& dest) {}

private:
	Func* _function;
};

template <>
class ModelFunc<Model::Mode::REAL> {
public:
	ModelFunc() : _address(nullptr) {}

	/// clone constructor
	ModelFunc(JB::IlBuilder* b, const ModelFunc<Model::Mode::REAL>& other)
		: _address(b->Copy(other._address)) {}

	void initialize(OMR_UNUSED JB::IlBuilder* b, Model::RUIntPtr function) {
		_address = function.unpack();
	}

	Model::RValue<CompiledFn> cbody(JB::IlBuilder* b) {
		return Model::RValue<CompiledFn>::pack(
			b->LoadIndirect("Func", "cbody", _address)
		);
	}

	Model::RSize nparams(JB::IlBuilder* b) {
		return Model::RSize::pack(
			b->LoadIndirect("Func", "nparams", _address)
		);
	}

	Model::RSize nlocals(JB::IlBuilder* b) {
		return Model::RSize::pack(
			b->LoadIndirect("Func", "nlocals", _address)
		);
	}

	Model::RPtr<std::uint8_t> body(JB::IlBuilder* b) {
		return Model::RPtr<std::uint8_t>::pack(
			b->StructFieldInstanceAddress("Func", "body", _address)
		);
	}

	JB::IlValue* unpack() { return _address; }

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, ModelFunc<Model::Mode::REAL>& dest) {}

	void reload(JB::IlBuilder* b) {}

private:
	JB::IlValue* _address;
};

/* RWY TODO: inherit from OMR::Model::Machine<M> */ 
template <Model::Mode M>
class Machine final : public JB::VirtualMachineState {
public:
	class Factory {
	public:
		void setInterpreter(JB::IlValue* interpreter) { _interpreter = interpreter; }

		void setFunction(Model::UIntPtr<M> function) { _function = function; }

		void setBuilders(JB::BytecodeBuilderTable* builders) { _builders = builders; }

		Machine<M>* create(JB::IlBuilder* b) {

			JB::TypeDictionary* t = b->typeDictionary();

			assert(_interpreter != nullptr);
			// TODO: assert(_function != nullptr);
	
			Machine<M>* machine = new Machine<M>(_builders);

			JB::IlValue* pcAddr      = b->StructFieldInstanceAddress("Interpreter", "_pc",      _interpreter);
			JB::IlValue* spAddr      = b->StructFieldInstanceAddress("Interpreter", "_sp",      _interpreter);
			JB::IlValue* fpAddr      = b->StructFieldInstanceAddress("Interpreter", "_fp",      _interpreter);
			JB::IlValue* startPcAddr = b->StructFieldInstanceAddress("Interpreter", "_startpc", _interpreter);

			machine->function.initialize(b, _function);
			machine->stack.initialize(b, t->Int64, spAddr);

			Model::Size<M> nlocals = machine->function.nlocals(b);
			JB::IlValue* localsAddr = machine->stack.reserve64(b, nlocals);
			machine->locals.initialize(b, t->Int64, localsAddr, nlocals);

			machine->pc.initialize(b, pcAddr, startPcAddr, machine->function.body(b));

			return machine;
		}

	private:
		JB::IlValue* _interpreter = nullptr;
		Model::UIntPtr<M> _function;
		JB::BytecodeBuilderTable* _builders = nullptr;
	};

	Machine(JB::BytecodeBuilderTable* builders) : pc(builders) {}

	Machine(const Machine&) = default;

	Machine(Machine&&) = default;

	void commit(JB::IlBuilder* b) {
		function.commit(b);
		stack.commit(b);
		locals.commit(b);
		pc.commit(b);
	}

	void mergeInto(JB::IlBuilder* b, Machine<M>& dest) {
		function.mergeInto(b, dest.function);
		stack.mergeInto(b, dest.stack);
		locals.mergeInto(b, dest.locals);
		pc.mergeInto(b, dest.pc);
	}

	void reload(JB::IlBuilder* b) {
		function.reload(b);
		stack.reload(b);
		pc.reload(b);
	}

	/// @group VirtualMachineState implementation
	/// @{

	virtual void Commit(JB::IlBuilder* b) override final {
		commit(b);
	}

	virtual void MergeInto(JB::VirtualMachineState* dest, JB::IlBuilder* b) override final {
		mergeInto(b, *reinterpret_cast<Machine<M>*>(dest));
	}

	virtual JB::VirtualMachineState* MakeCopy() override final {
		return new Machine<M>(*this);
	}

	/// @}
	///

	ModelFunc<M> function;
	Model::OperandStack<M> stack;
	Model::OperandArray<M> locals;
	Model::Pc<M> pc;

private:
	friend class Factory;

	Machine() {}
};

using RealMachine = Machine<Model::Mode::REAL>;
using VirtMachine = Machine<Model::Mode::VIRT>;

template <Model::Mode M>
struct GenDispatchValue;

template <>
struct GenDispatchValue<Model::Mode::REAL> {
	Model::RUIntPtr operator()(JB::IlBuilder* b, RealMachine& machine) {
		return Model::RUIntPtr::pack(
			b->LoadAt(b->typeDictionary()->pInt8,
				machine.pc.load(b).unpack()
		));
	}
};

template <>
struct GenDispatchValue<Model::Mode::VIRT> {
	Model::CUIntPtr operator()(JB::IlBuilder* b, VirtMachine& machine) {
		// translate pc to bytecode index.
		return Model::CUIntPtr::pack(
			static_cast<std::uintptr_t>(
				*reinterpret_cast<std::uint8_t*>(machine.pc.load(b).unpack())));
	}
};

void next(JB::IlBuilder* b, RealMachine& machine, Model::RSize offset) {
	JB::TypeDictionary* t = b->typeDictionary();
	machine.pc.next(b, offset);
	machine.commit(b);
}

void next(JB::IlBuilder* b, VirtMachine& machine, Model::CSize offset) {
	JB::TypeDictionary* t = b->typeDictionary();
	machine.pc.next(b, offset);
	machine.commit(b);
}

void halt(JB::IlBuilder* b, RealMachine& machine) {
	b->Call("print_s", 1, b->Const((void*)"HALT\n"));
	machine.commit(b);
	b->Return();
}

void halt(JB::IlBuilder* b, VirtMachine& machine) {
	b->Call("print_s", 1, b->Const((void*)"HALT\n"));
	machine.commit(b);
	b->Return();
}

void gen_dbg_msg(JB::IlBuilder* b, const char* file, std::size_t line, const char* func, const char* msg) {
	b->Call("dbg_msg", 4,
		Model::constant(b, file),
		Model::constant(b, line),
		Model::constant(b, func),
		Model::constant(b, msg)
	);
	// b->Call("interp_trace", 2, b->Load("interpreter"), b->Load("target"));
}

#define GEN_DBG_MSG(b, msg) gen_dbg_msg(b, __FILE__, __LINE__, __FUNCTION__, msg)

#define GEN_TRACE_MSG(b, msg) GEN_DBG_MSG(b, msg)

#define GEN_TRACE(b) GEN_DBG_MSG(b, "trace")

template <Model::Mode M>
struct GenFunctionEntry;

template <>
struct GenFunctionEntry<Model::Mode::REAL> {
	void operator()(JB::IlBuilder* b, RealMachine& machine) {
		OMR_TRACE();
		/// RWY TODO: Initializing the machine this late in the 
		/// method / interpreter is too late. The Func model has to be available
		/// at initialization time, so we can establish the compiled method's parameters.

		GEN_TRACE_MSG(b, "ENTER METHOD");

		JB::IlValue* interpreter = b->Load("interpreter");
		JB::IlValue* target = b->Load("target");

		assert(0); // need to figure out how to initialize the machine
#if 0
		RealMachine::Factory factory;
		factory.setInterpreter(interpreter);
		factory.setFunction(Model::RUIntPtr::pack(target));
		_machine = std::shared_ptr<Machine<M>>(factory.create(b));
		_machine->commit(b);
#endif

		GEN_TRACE_MSG(b, "MACHINE INITIALIZED");
		b->Call("interp_trace", 2, interpreter, target);
		// _machine->initialize(b);
	}
};

template <>
struct GenFunctionEntry<Model::Mode::VIRT> {
	void operator()(Model::VirtIlBuilder* b, VirtMachine& machine) {
		OMR_TRACE();
		/// RWY TODO: This function entry is unhandled.
		b->Call("print_s", 1, b->Const((void*)nullptr)); // abort
	}
};

template <Model::Mode M>
struct GenDefault {
	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "DEFAULT HANDLER");
		halt(b, machine);
		return true;
	}
};

template <Model::Mode M>
struct GenError {
	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "ERROR (UNKNOWN BYTECODE)");
		halt(b, machine);
		return true;
	}
};

template <Model::Mode M>
struct GenHalt {
	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		GEN_TRACE_MSG(b, "HALT");
		halt(b, machine);
		return true;
	}
};

template <Model::Mode M>
struct GenNop {
	static constexpr std::size_t INSTR_SIZE = 1;

	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "NOP");
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <Model::Mode M>
struct GenPushConst {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_CONST_OFFSET = 1;

	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "PUSH_CONST");
		Model::Int64<M> c = machine.pc.immediateInt64(b, Model::Size<M>(b, INSTR_CONST_OFFSET));

		b->Call("print_s", 1, b->Const((void*)"PushConstBuilder: build: value="));
		b->Call("print_u", 1, c.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		machine.stack.pushInt64(b, c.toIl(b));

		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <Model::Mode M>
struct GenAdd {
	static constexpr std::size_t INSTR_SIZE = 1;

	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "ADD");
		JB::IlValue* rhs = machine.stack.popInt64(b);
		JB::IlValue* lhs = machine.stack.popInt64(b);
		machine.stack.pushInt64(b, b->Add(lhs, rhs));
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <Model::Mode M>
struct GenPushLocal {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "PUSH_LOCAL");
		Model::Size<M> index = machine.pc.immediateSize(b, Model::Size<M>(b, INSTR_INDEX_OFFSET));
		JB::IlValue* value = machine.locals.get(b, index);
		machine.stack.pushInt64(b, value);
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

template <Model::Mode M>
struct GenPopLocal {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	bool operator()(Model::IlBuilder<M>* b, Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "POP_LOCAL");
		Model::Size<M> index = machine.pc.immediateSize(b, Model::Size<M>(b, INSTR_INDEX_OFFSET));
		machine.locals.set(b, index, machine.stack.popInt64(b));
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
		return true;
	}
};

#if 0
template <Model::Mode M>
struct BranchBuilder {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_TARGET_OFFSET = 1;

	void build(Machine<M>& machine, Model::IlBuilder<M>* b) {
		Model::UInt<M> target = machine.pc.immediateUInt(b, Model::UInt<M>(b, INSTR_TARGET_OFFSET));
		JB::IlValue* value = machine.locals.at(b, index);
		machine.stack.push(b, value);
		// todo: machine.pc.next(b, Model::add(b, Model::Size<M>(b, INSTR_SIZE), target));
	}
};
#endif // 0

struct CallBuilderBase {
protected:
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t TARGET_OFFSET = 1;
};

template <Model::Mode>
struct CallBuilder;

template <>
struct CallBuilder<Model::Mode::VIRT> : CallBuilderBase {
	static void build(Machine<Model::Mode::VIRT>& machine, JB::IlBuilder* b) {
		// TODO! machine.pc.next(b, Model::CSize(b, INSTR_SIZE));
	}
};

template <>
struct CallBuilder<Model::Mode::REAL> : CallBuilderBase {
	void build(Machine<Model::Mode::REAL>& machine, JB::IlBuilder* b) {
		machine.pc.next(b, Model::RSize(b, INSTR_SIZE));
	}
};

class InterpreterCompiler {
public:
	static constexpr Model::Mode M = Model::Mode::REAL;

	using HandlerTable = JB::BytecodeInterpreterBuilder::HandlerTable<Machine<M>>;

	InterpreterCompiler() {
		JitTypes::define(&_typedict);
		set(Op::UNKNOWN,    GenError<M>());
		set(Op::NOP,        GenNop<M>());
		set(Op::HALT,       GenHalt<M>());
		set(Op::PUSH_CONST, GenPushConst<M>());
		set(Op::ADD,        GenAdd<M>());
		set(Op::PUSH_LOCAL, GenPushLocal<M>());
		set(Op::POP_LOCAL,  GenPopLocal<M>());

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

		Machine<M>::Factory factory;
		factory.setInterpreter(interpreter);
		factory.setFunction(Model::RUIntPtr::pack(target));
		_machine.reset(factory.create(this));
		_machine->commit(this);

		GEN_TRACE_MSG(this, "$$$ MACHINE INITIALIZED");
		Call("interp_trace", 2, interpreter, target);

		bool success = buildInterpreterIL(_machine.get()); // dispatch to superclass

		GEN_TRACE_MSG(this, "$$$ EXIT METHOD");
		Return();
		return success;
	}

private:
	std::unique_ptr<Machine<Model::Mode::REAL>> _machine;
};

class MethodCompiler {
public:
	using BytecodeHandlerTable = JB::BytecodeHandlerTable<Machine<Model::Mode::VIRT>>;

	MethodCompiler() : _typedict() {
		JitTypes::define(&_typedict);

		constexpr Model::Mode M = Model::Mode::VIRT;

		set(Op::UNKNOWN,    GenError<M>());
		set(Op::NOP,        GenNop<M>());
		set(Op::HALT,       GenHalt<M>());
		set(Op::PUSH_CONST, GenPushConst<M>());
		set(Op::ADD,        GenAdd<M>());
		set(Op::PUSH_LOCAL, GenPushLocal<M>());
		set(Op::POP_LOCAL,  GenPopLocal<M>());
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
		VirtMachine::Factory factory;
		factory.setInterpreter(Load("interpreter"));
		factory.setFunction(Model::CUIntPtr::pack(std::uintptr_t(_func)));
		factory.setBuilders(builders());
		std::shared_ptr<VirtMachine> machine(factory.create(this));
		setVMState(machine.get());

		return buildBytecodeIL();
	}

private:
	Func* _func;
};

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

CompiledFn compile(Func* func) {
	MethodCompiler compiler;
	BytecodeMethodBuilder builder(&compiler, func);
	void* result = nullptr;
	std::int32_t rc = compileMethodBuilder(&builder, &result);
	assert(rc == 0);
	return (CompiledFn)result;
}

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

extern "C" int main(int argc, char** argv) {
	initializeJit();

#if 0
	Interpreter interpreter;
	Func* target = MakePopThenPushToLocalFunction();
	CompiledFn cfunc = compile(target);
	cfunc(&interpreter);
	free(target);
#else
	InterpretFn interpret = buildInterpret();

	Func* target = MakePopThenPushToLocalFunction();
	Interpreter interpreter;

	fprintf(stderr, "int main: interpreter=%p\n", &interpreter);
	fprintf(stderr, "int main: target=%p\n", target);
	fprintf(stderr, "int main: target startpc=%p\n", &target->body[0]);
	fprintf(stderr, "int main: initial op=%hhu\n", target->body[0]);
	fprintf(stderr, "int main: initial sp=%p\n", interpreter.sp());
	interpret_wrap(&interpreter, target, interpret);

	free(target);
#endif

	fprintf(stderr, "int main: stack[0]=%llu\n", interpreter.peek(0));
	return 0;
}
