#include <stdint.h> // HACK
#include <stddef.h> // HACK

//////////////////////////////////

#include <OMR/ByteBuffer.hpp>

#include <OMR/Model/Value.hpp>
#include <OMR/Model/OperandStack.hpp>
#include <OMR/Model/OperandArray.hpp>
#include <OMR/Model/Register.hpp>
#include <OMR/Model/Pc.hpp>
#include <OMR/Model/Builder.hpp>

#include <VirtualMachineState.hpp>
#include <TypeDictionary.hpp>
#include <JitBuilder.hpp>

#include "JitHelpers.hpp"
#include "JitTypes.hpp"
#include "Interpreter.hpp"

#include <cstdint>
#include <cassert>

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

	void mergeInto(JB::IlBuilder* b, ModelFunc<Model::Mode::VIRT>* dest) {}

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
			JB::IlValue* startPcAddr = b->StructFieldInstanceAddress("Interpreter", "_startpc", _interpreter);

			machine.function.initialize(b, _function);
			machine.stack.initialize(b, t->Int64, spAddr);

			Model::Size<M> nlocals    = machine.function.nlocals(b);
			// JB::IlValue*   localsAddr = machine.stack.reserve64(b, nlocals);

			// machine.locals.initialize(b, t->Int64, localsAddr, nlocals);

			machine.pc.initialize(b, pcAddr, startPcAddr, machine.function.body(b));

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
		// todo: locals.commit(b);
		pc.commit(b);
	}

	void mergeInto(JB::IlBuilder* b, Machine<M>& dest) {
		function.mergeInto(b, dest.function);
		stack.mergeInto(b, dest.stack);
		// todo: locals
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
	// Model::OperandArray<M> locals;
	Model::Pc<M> pc;

private:
	friend class Factory;

	Machine() {}
};

using RealMachine = Machine<Model::Mode::REAL>;
using VirtMachine = Machine<Model::Mode::VIRT>;

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

void next(JB::IlBuilder* b, RealMachine& machine, Model::RSize offset) {

	JB::TypeDictionary* t = b->typeDictionary();

	machine.pc.next(b, offset);

	InstructionDispatch<Model::Mode::REAL> dispatch;
	JB::IlValue* target = dispatch.target(b, machine).unpack();
	JB::IlValue* target32 = b->ConvertTo(t->Int32, target);

	b->Call("print_s", 1, b->Const((char*)"NEXT: next-bc="));
	b->Call("print_x", 1, target);
	b->Call("print_s", 1, b->Const((void*)"\nNEXT: dispatch: converted-next-bc="));
	b->Call("print_x", 1, target32);
	b->Call("print_s", 1, b->Const((void*)"\n"));

	b->Store("opcode", target32);
	machine.commit(b);
}

void halt(JB::IlBuilder* b, RealMachine& machine) {
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
	b->Call("interp_trace", 2, b->Load("interpreter"), b->Load("target"));
}

#define GEN_DBG_MSG(b, msg) gen_dbg_msg(b, __FILE__, __LINE__, __FUNCTION__, msg)

#define GEN_TRACE_MSG(b, msg) GEN_DBG_MSG(b, msg)

#define GEN_TRACE(b) GEN_DBG_MSG(b, "trace")

template <Model::Mode M>
struct FunctionEntryBuilder {
	static constexpr std::size_t LOCAL_SIZE = 4;
	void build(Machine<M>& machine, JB::IlBuilder* b) {
		machine.stack.reserve64(b, mul(b, machine.pc.function().nlocals(), LOCAL_SIZE));
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
		// TODO! machine.pc.next(b, Model::CSize(b, INSTR_SIZE));
	}
};

template <>
struct CallBuilder<Model::Mode::REAL> : CallBuilderBase {
	void build(Machine<Model::Mode::REAL>& machine, JB::IlBuilder* b) {
		machine.pc.next(b, Model::RSize(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct HaltBuilder {
	static constexpr std::size_t INSTR_SIZE = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE_MSG(b, "HALT");
		halt(b, machine);
	}
};

template <Model::Mode M>
struct NopBuilder {
	static constexpr std::size_t INSTR_SIZE = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE_MSG(b, "NOP");
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct PushConstBuilder {
	static constexpr std::size_t INSTR_SIZE = 9;
	static constexpr std::size_t INSTR_CONST_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE_MSG(b, "PUSH_CONST");
		Model::Int64<M> c = machine.pc.immediateInt64(b, Model::Size<M>(b, INSTR_CONST_OFFSET));

		b->Call("print_s", 1, b->Const((void*)"PushConstBuilder: build: value="));
		b->Call("print_u", 1, c.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		machine.stack.pushInt64(b, c.toIl(b));

		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct AddBuilder {
	static constexpr std::size_t INSTR_SIZE = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE_MSG(b, "ADD");
		JB::IlValue* rhs = machine.stack.popInt64(b);
		JB::IlValue* lhs = machine.stack.popInt64(b);
		machine.stack.pushInt64(b, b->Add(lhs, rhs));
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct PushLocalBuilder {
	static constexpr std::size_t INSTR_SIZE = 5;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE(b);
		Model::UInt64<M> index = machine.pc.immediateUInt64(b, Model::UInt<M>(b, INSTR_INDEX_OFFSET));
		JB::IlValue* value = machine.locals.at(b, index);
		machine.stack.pushInt64(b, value);
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
	}
};

template <Model::Mode M>
struct PopLocalBuilder {
	static constexpr std::size_t INSTR_SIZE = 5;
	static constexpr std::size_t INSTR_INDEX_OFFSET = 1;

	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE(b);
		Model::UInt<M> index = machine.pc.immediateUInt(b, Model::UInt<M>(b, INSTR_INDEX_OFFSET));
		machine.locals.set(b, index, machine.stack.popUInt(b));
		next(b, machine, Model::Size<M>(b, INSTR_SIZE));
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
		// machine.pc.next(b, Model::add(b, Model::Size<M>(b, INSTR_SIZE), target));
	}
};

template <Model::Mode M>
struct DefaultHandler {
	void build(Machine<M>& machine, JB::IlBuilder* b) {
		GEN_TRACE_MSG(b, "DEFAULT");
		halt(b, machine);
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
	Compiler() :
		_typedict() {
		JitTypes::define(&_typedict);
	}

	JB::TypeDictionary* typedict() { return &_typedict; }

private:
	JB::TypeDictionary _typedict;
};

template <typename InsnBuilderT>
struct BuildInsn {
	void build() {
		// InsnBuilderT handler;
		// handler.buildIL(machine.MakeCopy(), builder);
	}
};

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

		//////////////////////////////////////

		Machine<Model::Mode::REAL>::Factory factory;
		factory.setInterpreter(Load("interpreter"));
		factory.setFunction(Model::RUIntPtr::pack(Load("target")));
		Machine<Model::Mode::REAL> machine = factory.build(this);

		//////////////////////////////////////

		DefineLocal("opcode", t->Int32);
		Store("opcode", Const(std::int32_t(0)));
	
		DefineLocal("continue", t->Int32);
		Store("continue", Const(std::int32_t(1)));

		AllLocalsHaveBeenDefined();

		//////////////////////////////////////

		// machine.commit(this);

		Call("interp_trace", 2, Load("interpreter"), Load("target"));

		//////////////////////////////////////

		{
			InstructionDispatch<Model::Mode::REAL> dispatch;
			JB::IlValue* target = dispatch.target(this, machine).unpack();
			Store("opcode", ConvertTo(t->Int32, target));
		}
	
		//////////////////////////////////////

		JB::IlBuilder* defaultHandler = OrphanBuilder();
		{
			Machine<Model::Mode::REAL> clone(machine);
			clone.reload(defaultHandler);
			DefaultHandler<Model::Mode::REAL>().build(clone, defaultHandler);
			// clone.commit(defaultHandler);
			// clone.mergeInto(defaultHandler, machine);
		}

		//////////////////////////////////////

		std::vector<JB::IlBuilder::JBCase*> cases;

		{ //////////// HALT
			IlBuilder* builder = nullptr;
			cases.push_back(MakeCase(std::int32_t(Op::HALT), &builder, false));
			Machine<Model::Mode::REAL> clone = machine;
			clone.reload(builder);
			HaltBuilder<Model::Mode::REAL>().build(clone, builder);
		}

		{ //////////// NOP
			IlBuilder* builder = nullptr;
			cases.push_back(MakeCase(std::int32_t(Op::NOP), &builder, false));
			Machine<Model::Mode::REAL> clone = machine;
			clone.reload(builder);
			NopBuilder<Model::Mode::REAL>().build(clone, builder);
			clone.commit(builder);
		}

		{ //////////// PUSH_CONST
			IlBuilder* builder = nullptr;
			cases.push_back(MakeCase(std::int32_t(Op::PUSH_CONST), &builder, false));
			Machine<Model::Mode::REAL> clone = machine;
			clone.reload(builder);
			PushConstBuilder<Model::Mode::REAL>().build(clone, builder);
		}

		{ //////////// ADD
			IlBuilder* builder = nullptr;
			cases.push_back(MakeCase(std::int32_t(Op::ADD), &builder, false));
			Machine<Model::Mode::REAL> clone = machine;
			clone.reload(builder);
			AddBuilder<Model::Mode::REAL>().build(clone, builder);
		}
	
		//////////////////////////////////////

		JB::IlBuilder* body = OrphanBuilder();

		DoWhileLoop((char*)"continue", &body);
		
		body->Call("print_s", 1, body->Const((void*)"*** WHILE LOOP START: opcode="));
		body->Call("print_u", 1, body->Load("opcode"));
		// body->Call("print_s", 1, body->Const((void*)" pc="));
		// body->Call("print_x", 1, machine.pc.load(body).unpack());
		body->Call("print_s", 1, body->Const((void*)"\n"));

		body->Switch("opcode", &defaultHandler, cases.size(), cases.data());

		//////////////////////////////////////

		Return();
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

Func* MakeAddConstsFunction() {
	OMR::ByteBuffer buffer;
	buffer << Func();
	buffer << Op::PUSH_CONST << std::int64_t(333);
	buffer << Op::PUSH_CONST << std::int64_t(444);
	buffer << Op::ADD;
	buffer << Op::HALT;
	return (Func*)buffer.release();
}

void interpret_wrap(Interpreter* interpreter, Func* target, InterpretFn interpret) {
	interpret(interpreter, target);
}

extern "C" int main(int argc, char** argv) {
	initializeJit();

	InterpretFn interpret = buildInterpret();

	Func* target = MakeAddConstsFunction();
	Interpreter interpreter;

	fprintf(stderr, "int main: interpreter=%p\n", &interpreter);
	fprintf(stderr, "int main: target=%p\n", target);
	fprintf(stderr, "int main: target startpc=%p\n", &target->body[0]);
	fprintf(stderr, "int main: initial op=%hhu\n", target->body[0]);
	fprintf(stderr, "int main: initial sp=%p\n", interpreter.sp());
	interpret_wrap(&interpreter, target, interpret);

	fprintf(stderr, "int main: stack[0]=%llu\n", interpreter.peek(0));

	free(target);
	return 0;
}
