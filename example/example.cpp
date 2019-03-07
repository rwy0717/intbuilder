#include <stdint.h> // HACK
#include <stddef.h> // HACK

//////////////////////////////////

#include <OMR/Model/Value.hpp>
#include <OMR/Model/OperandStack.hpp>
#include <OMR/Model/OperandArray.hpp>
#include <OMR/Model/Register.hpp>
#include <OMR/Model/Pc.hpp>
#include <OMR/Model/Builder.hpp>

#include <VirtualMachineState.hpp>
#include <TypeDictionary.hpp>
#include <JitBuilder.hpp>

#include <OMR/ByteBuffer.hpp>

#include <cstdint>
#include <cassert>

namespace Model = OMR::Model;
namespace JB = OMR::JitBuilder;

enum class Op : std::uint8_t {
	HALT, NOP, PUSH_CONST, ADD, PUSH_LOCAL, POP_LOCAL, CALL,
	LAST = CALL
};

constexpr std::size_t OPCOUNT = std::size_t(Op::LAST);

class Interpreter;

using CompiledFn = void(*)(Interpreter*);

/// Function header.
struct Func {
	CompiledFn cbody = nullptr; //< compiled body ptr.
	std::size_t nlocals = 0;
	std::size_t nparams = 0;
	std::uint8_t body[]; //< bytecode body. trailing data.
};

void defineFunc(JB::TypeDictionary* t) {
	t->DefineStruct("Func");
	t->DefineField("Func", "cbody",   t->Address, offsetof(Func, cbody));
	t->DefineField("Func", "nlocals", t->Word,    offsetof(Func, nlocals));
	t->DefineField("Func", "nparams", t->Word,    offsetof(Func, nparams));
	t->DefineField("Func", "body",    t->NoType,  offsetof(Func, body));
	t->CloseStruct("Func");
}

/// The main interpreter function type. Generated by JitBuilder.
using InterpretFn = void(*)(Interpreter*, Func*);

/// The interpreter state.
struct Interpreter {
public:
	static constexpr std::size_t  STACK_SIZE = 1024; //< in bytes
	static constexpr std::uint8_t POISON     = 0x5e;

	Interpreter() :
		_sp(nullptr), _pc(nullptr), _fp(nullptr), _interpret(nullptr) {
		memset(_stack, POISON, STACK_SIZE);
	}

	void run(Func* target) {
		if (target->cbody != nullptr) {
			target->cbody(this);
		} else {
			interpret(target);
		}
	}

	void interpret(Func* target) {
		assert(_interpret != nullptr);
		_interpret(this, target);
	}

// private:
	friend struct JitHelpers;
	friend void defineInterpreter(JB::TypeDictionary*);

	std::uint8_t* _sp;                //< Stack pointer. Pointer to top of stack.
	std::uint8_t* _pc;                //< Program counter. Pointer to current bytecode.
	std::uint8_t* _startpc;           //< pc at function entry. Used for absolute jumps.
	Func* _fp;                        //< Function pointer. Pointer to current function.
	InterpretFn _interpret;           //< Interpreter main function. Generated by JitBuilder.
	std::uint8_t _stack[STACK_SIZE];
};

void defineInterpreter(JB::TypeDictionary* t) {
	t->DefineStruct("Interpreter");
	t->DefineField("Interpreter", "_sp",        t->pInt64,                             offsetof(Interpreter, _sp));
	t->DefineField("Interpreter", "_pc",        t->pInt8,                              offsetof(Interpreter, _pc));
	t->DefineField("Interpreter", "_startpc",   t->pInt8,                              offsetof(Interpreter, _startpc));
	t->DefineField("Interpreter", "_fp",        t->PointerTo(t->LookupStruct("Func")), offsetof(Interpreter, _fp));
	t->DefineField("Interpreter", "_interpret", t->Address,                            offsetof(Interpreter, _interpret));
	t->DefineField("Interpreter", "_stack",     t->NoType,                             offsetof(Interpreter, _stack));
	t->CloseStruct("Interpreter");
}

/// Print a mini trace statement.
static void jit_trace(Interpreter* interpreter, Func* func) {
	fprintf(stderr, "$$$ interpreter=%p func=%p: pc=%p=%hhu\n",
		interpreter, func, interpreter->_pc, *interpreter->_pc);
}

struct JitHelpers {
public:
	static void define(JB::MethodBuilder* b) {
		JB::TypeDictionary* t = b->typeDictionary();

		b->DefineFunction("jit_trace", "1234", "jit_helper", (void *)&jit_trace, t->NoType, 2,
			t->PointerTo(t->LookupStruct("Interpreter")),
			t->PointerTo(t->LookupStruct("Func"))
		);

		b->DefineFunction("print_string", "", "", (void *)&print_string, t->NoType, 1,
			t->PointerTo(t->Int8)
		);
	}

private:
	/// Run a function in the interpreter.
	static void run(Interpreter* interpreter, Func* target) {
		interpreter->run(target);
	}
	
	///
	static void print_string(const char* str) {
		fprintf(stderr, "%s\n", str);
	}
};

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

	Model::CUIntPtr body(OMR_UNUSED JB::IlBuilder* b) {
		return Model::CUIntPtr::pack(
			reinterpret_cast<std::uintptr_t>(&_function->body)
		);
	}

	Model::CValue<CompiledFn> cbody(OMR_UNUSED JB::IlBuilder* b) {
		return Model::CValue<CompiledFn>::pack(_function->cbody);
	}

	Func* unpack() { return _function; }

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, ModelFunc<Model::Mode::VIRT>* dest) {}

private:
	Func* _function;
};

template <>
class ModelFunc<Model::Mode::REAL> {
public:
	ModelFunc() : _address(nullptr) {}

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


	Model::RUIntPtr body(JB::IlBuilder* b) {
		return Model::RUIntPtr::pack(
			b->StructFieldInstanceAddress("Func", "body", _address)
		);
	}

	JB::IlValue* unpack() { return _address; }

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, ModelFunc<Model::Mode::REAL>* dest) {}

private:
	JB::IlValue* _address;
};

template <Model::Mode M>
class Machine final : public JB::VirtualMachineState {
public:
	class Factory {
	public:
		void setInterpreter(JB::IlValue* interpreter) { _interpreter = interpreter; }

		void setFunction(Model::UIntPtr<M> function) { _function = function; }

		Machine<M> build(JB::IlBuilder* b) {

			JB::TypeDictionary* t = b->typeDictionary();

			assert(_interpreter != nullptr);
			// TODO: assert(_function != nullptr);
	
			Machine<M> machine;

			JB::IlValue* pcAddr      = b->StructFieldInstanceAddress("Interpreter", "_pc",      _interpreter);
			JB::IlValue* spAddr      = b->StructFieldInstanceAddress("Interpreter", "_sp",      _interpreter);
			JB::IlValue* fpAddr      = b->StructFieldInstanceAddress("Interpreter", "_fp",      _interpreter);
			JB::IlValue* startpcAddr = b->StructFieldInstanceAddress("Interpreter", "_startpc", _interpreter);

			machine.function.initialize(b, _function);
			machine.stack.initialize(b, t->Int64, spAddr);

			Model::Size<M> nlocals    = machine.function.nlocals(b);
			JB::IlValue*   localsAddr = machine.stack.reserve(b, nlocals);

			machine.locals.initialize(b, t->Int64, localsAddr, nlocals);

			machine.pc.initialize(b, pcAddr, startpcAddr, machine.function.body(b));

			return machine;
		}

	private:
		JB::IlValue* _interpreter = nullptr;
		Model::UIntPtr<M> _function;
	};

	Machine(const Machine&) = default;

	Machine(Machine&&) = default;

	void commit(JB::IlBuilder* b) {
		function.commit(b);
		stack.commit(b);
		locals.commit(b);
		pc.commit(b);
	}

	ModelFunc<M> function;
	Model::OperandStack<M> stack;
	Model::OperandArray<M> locals;
	Model::Pc<M> pc;

private:
	friend class Factory;

	Machine() {}
};

void gentrace(JB::IlBuilder* b, const char* str) {
	b->Call("print_string", 1, b->Const((void*)str));
}

#define GENTRACE(b) gentrace(b, __PRETTY_FUNCTION__)

template <Model::Mode M>
struct FunctionEntryBuilder {
	static constexpr std::size_t LOCAL_SIZE = 4;
	void build(Machine<M>& machine, JB::IlBuilder* b) {
		machine.stack.reserve(b, mul(b, machine.pc.function().nlocals(), LOCAL_SIZE));
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
	void build(Machine<Model::Mode::VIRT>& machine, JB::IlBuilder* b) {
		machine.pc.next(b, Model::CUInt(b, INSTR_SIZE));
	}
};

template <>
struct CallBuilder<Model::Mode::REAL> : CallBuilderBase {
	void build(Machine<Model::Mode::REAL>& machine, JB::IlBuilder* b) {
		machine.pc.next(b, Model::RUInt(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct HaltBuilder {
	static constexpr std::size_t INSTR_SIZE = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GENTRACE(b);
		machine.pc.halt(b);
	}
};

template <Model::Mode M>
struct NopBuilder {
	static constexpr std::size_t INSTR_SIZE = 1;

	void build(Machine<M> machine, JB::IlBuilder* b) {
		GENTRACE(b);
		machine.pc.next(b, Model::UInt<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct PushConstBuilder {
	static constexpr std::size_t INSTR_SIZE = 5;
	static constexpr std::size_t INSTR_CONST_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		Model::UInt<M> c = machine.pc.immediateInt(b, Model::UInt<M>(b, INSTR_CONST_OFFSET));
		machine.stack.push(b, c.toIl(b));
		machine.pc.next(b, Model::UInt<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct AddBuilder {
	static constexpr std::size_t INSTR_SIZE = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		JB::IlValue* rhs = machine.stack.popInt(b);
		JB::IlValue* lhs = machine.stack.popInt(b);
		machine.stack.push(b, b->Add(lhs, rhs));
		machine.pc.next(b, Model::UInt<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct PushLocalBuilder {
	static constexpr std::size_t INSTR_SIZE = 5;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		Model::UInt<M> index = machine.pc.immediateUInt(b, Model::UInt<M>(b, INSTR_INDEX_OFFSET));
		JB::IlValue* value = machine.locals.at(b, index);
		machine.stack.push(b, value);
		machine.pc.next(b, Model::UInt<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct PopLocalBuilder {
	static constexpr std::size_t INSTR_SIZE = 5;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		Model::UInt<M> index = machine.pc.immediateUInt(b, Model::UInt<M>(b, INSTR_INDEX_OFFSET));
		machine.locals.set(b, index, machine.stack.popUInt(b));
		machine.pc.next(b, Model::UInt<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct BranchBuilder {
	static constexpr std::size_t INSTR_SIZE = 5;
	static constexpr std::size_t INSTR_TARGET_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		Model::UInt<M> target = machine.pc.immediateUInt(b, Model::UInt<M>(b, INSTR_TARGET_OFFSET));
		JB::IlValue* value = machine.locals.at(b, index);
		machine.stack.push(b, value);
		machine.pc.next(b, Model::add(b, Model::UInt<M>(b, INSTR_SIZE), target));
	}
};

template <Model::Mode M>
struct InstructionDispatch;

template <>
struct InstructionDispatch<Model::Mode::REAL> {
	Model::RUIntPtr target(JB::IlBuilder* b, Machine<Model::Mode::REAL>& machine) {
		return Model::RUIntPtr::pack(
			b->LoadAt(b->typeDictionary()->pInt8, machine.pc.load(b).unpack()));
	}
};

template <>
struct InstructionDispatch<Model::Mode::VIRT> {
	Model::CUIntPtr target(JB::IlBuilder* b, Model::CUIntPtr pc) {
		// translate pc to bytecode index.
		return Model::CUIntPtr::pack(
			static_cast<std::uintptr_t>(
				*reinterpret_cast<std::uint8_t*>(pc.unpack())));
	}
};

template <Model::Mode M>
struct DefaultBuilder {
	void build(Machine<M> machine, JB::IlBuilder* b) {
		GENTRACE(b);
		b->Return();
	}
};

template <Model::Mode M>
struct InstructionSet {
	template <typename Instruction>
	void buildInstruction(Instruction& instruction, JB::IlBuilder* b) {
		// Machine<M> machine(
		// 	b->StructFieldInstanceAddress("Interpreter", "_sp", interpreter))
		// );
		// machine.initialize(b);
		// instruction.build(machine, b);
	}
	// void notifySet(InstructionTable<M>& table) {
	// 	// table.register((unsigned int)Op::PUSH_CONST, new PushConstBuilder<M>());
	// 	// table.register((unsigned int)Op::ADD,        new AddBuilder<M>());
	// 	// table.register((unsigned int)Op::POP_LOCAL,  new PopLocalBuilder<M>());
	// 	// table.register((unsigned int)Op::PUSH_LOCAL, new PushLocalBuilder<M>());
	// }
};

class Compiler {
public:
	Compiler() : _typedict() { defineTypes(); }

	JB::TypeDictionary* typedict() { return &_typedict; }

private:
	void defineTypes() {
		defineFunc(typedict());
		defineInterpreter(typedict());
	}

	JB::TypeDictionary _typedict;
};

/// Define all types and structs in jitbuilder
///

// struct CaseEntry {
// 	std::vector<std::int32_t> value;
// 	std::
// };

// struct CaseTable {
// public:
// 	CaseTable(std::size_t size = 0) {}

// 	build(JB::IlBuilder* b, std::string local) {

// 	}

// 	set(std::int32_t value, JB::IlBuilder* handler, bool fallthrough = false) {

// 	}

// 	// at(std::int32_t i)

// private:
// 	std::vector<std::int32_t> values;
// 	std::vector<bool> fallthrough;
// 	std::vector<JB::IlBuilder*> body;
// };

class InterpreterBuilder : public JB::MethodBuilder {
public:
	InterpreterBuilder(Compiler& compiler) :
		JB::MethodBuilder(compiler.typedict()),
		_compiler(compiler) {

		OMR_TRACE();

		JB::TypeDictionary* t = _compiler.typedict();
	
		DefineName("interpret");
		DefineLine("0");
		DefineFile("<generated>");

		DefineParameter("interpreter", t->PointerTo(t->LookupStruct("Interpreter")));
		DefineParameter("target", t->PointerTo(t->LookupStruct("Func")));
		DefineReturnType(t->NoType);
	}

	virtual bool buildIL() override final {

		OMR_TRACE();

		JB::TypeDictionary* t = _compiler.typedict();
	
		JitHelpers::define(this);

		Machine<Model::Mode::REAL>::Factory factory;
		factory.setInterpreter(Load("interpreter"));
		factory.setFunction(Model::RUIntPtr::pack(Load("target")));
		Machine<Model::Mode::REAL> machine = factory.build(this);

		DefineLocal("opcode", t->Int32);

		AllLocalsHaveBeenDefined();


		machine.commit(this);

		Call("jit_trace", 2, Load("interpreter"), Load("target"));


		/////////////////////////////////

		InstructionDispatch<Model::Mode::REAL> dispatch;
		JB::IlValue* target = dispatch.target(this, machine).unpack();
		Store("opcode", ConvertTo(t->Int32, target));
	
		//////////////////////////////////////

		JB::IlBuilder* defaultx = OrphanBuilder();
		DefaultBuilder<Model::Mode::REAL>().build(machine, defaultx);

		// TR::IlValue *pc = getPC(doWhileBody);
		// TR::IlValue *increment = doWhileBody->ConstInt32(1);
		// TR::IlValue *newPC = doWhileBody->Add(pc, increment);
		// setPC(doWhileBody, newPC);

		std::vector<JB::IlBuilder::JBCase*> cases;

		{
			IlBuilder* builder = nullptr;
			cases.push_back(MakeCase(std::int32_t(Op::HALT), &builder, false));
			HaltBuilder<Model::Mode::REAL>().build(machine, builder);
		}

		{
			IlBuilder* builder = nullptr;
			cases.push_back(MakeCase(std::int32_t(Op::NOP), &builder, false));
			NopBuilder<Model::Mode::REAL>().build(machine, builder);
		}

		Switch("opcode", &defaultx, cases.size(), cases.data());
	
		GENTRACE(this);
		Return();
		// size_t numberOfRegisteredBytecodes = _registeredBytecodes.size();
		// int32_t *caseValues = (int32_t *) _comp->trMemory()->allocateHeapMemory(numberOfRegisteredBytecodes * sizeof(int32_t));
		// TR_ASSERT(0 != caseValues, "out of memory");

		// TR::BytecodeBuilder **caseBuilders = (TR::BytecodeBuilder **) _comp->trMemory()->allocateHeapMemory(numberOfRegisteredBytecodes * sizeof(TR::BytecodeBuilder *));
		// TR_ASSERT(0 != caseBuilders, "out of memory");



		// std::sort(_registeredBytecodes.begin(), _registeredBytecodes.end(), bytecodeBuilderSort);

		// for (int i = 0; i < OPCOUNT; i++) {
		// 	TR::BytecodeBuilder *builder = _registeredBytecodes.at(i);
		// 	caseBuilders[i] = builder;
		// 	caseValues[i] = builder->bcIndex();
		// 	caseFallsThrough[i] = false;
		// }

		// Switch("opcode", doWhileBody, _defaultHandler, numberOfRegisteredBytecodes, caseValues, caseBuilders, caseFallsThrough);

		// _defaultHandler->Call("handleBadOpcode", 2, _defaultHandler->Load(_opcodeName), _defaultHandler->Load(_pcName));
		// _defaultHandler->Goto(&breakBody);

//    handleInterpreterExit(this);

	// return true;
			/////////////////////////////////////
		// 	Switch()
		// Return();

		return true;
	}

private:
	Compiler& _compiler;
};

/// Create the interpret function.
///
InterpretFn buildInterpret() {
	Compiler compiler;
	InterpreterBuilder builder(compiler);
	void* interpret = nullptr;
	std::int32_t rc = compileMethodBuilder(&builder, &interpret);
	return (InterpretFn)interpret;
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

Func* MakeAddConstsFunc() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int32_t(1);
	buffer << Op::PUSH_CONST << std::int32_t(1);
	buffer << Op::ADD;
	return (Func*)buffer.release();
}

extern "C" int main(int argc, char** argv) {
	initializeJit();

	InterpretFn interpret = buildInterpret();

	Func* target = MakeNopThenHaltFunction();
	Interpreter interpreter;

	fprintf(stderr, "int main: interpreter=%p\n", &interpreter);
	fprintf(stderr, "int main: target=%p\n", target);
	fprintf(stderr, "int main: target startpc=%p\n", &target->body[0]);
	fprintf(stderr, "int main: initial op=%hhu\n", target->body[0]);
	interpret(&interpreter, target);
	// free(target);
	return 0;
}
