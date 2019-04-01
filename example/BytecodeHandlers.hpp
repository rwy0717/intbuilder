#if !defined(BYTECODEHANDLERS_HPP_)
#define BYTECODEHANDLERS_HPP_

#include <stdint.h> // HACK
#include <stddef.h> // HACK

//////////////////////////////////

#include <JitHelpers.hpp>
#include <JitTypes.hpp>
#include <Interpreter.hpp>
#include <Model.hpp>

#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Builder.hpp>

#include <VirtualMachineState.hpp>
#include <TypeDictionary.hpp>
#include <JitBuilder.hpp>
#include <IlBuilder.hpp>

#include <cstdint>
#include <cassert>
#include <memory>

namespace JB = OMR::JitBuilder;

inline void gen_dbg_msg(JB::IlBuilder* b, const char* file, std::size_t line, const char* func, const char* msg) {
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

template <>
struct GenDispatchValue<OMR::Model::Mode::REAL> {
	OMR::Model::RUIntPtr operator()(JB::IlBuilder* b, Model::RealMachine& machine) {
		return OMR::Model::RUIntPtr::pack(
			b->LoadAt(b->typeDictionary()->pInt8,
				machine.instruction.xaddress(b).unpack()));
	}
};

template <>
struct GenDispatchValue<Model::Mode::VIRT> {
	Model::CUIntPtr operator()(JB::CBuilder* b, Model::VirtMachine& machine) {
		// translate pc to bytecode index.
		return OMR::Model::CUIntPtr::pack(
			static_cast<std::uintptr_t>(
				*reinterpret_cast<std::uint8_t*>(machine.instruction.address(b).unpack())));
	}
};

template <OMR::Model::Mode M>
struct GenFunctionEntry;

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

template <>
struct GenFunctionEntry<OMR::Model::Mode::VIRT> {
	void operator()(JB::IlBuilder* b, Model::VirtMachine& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "FUNCTION ENTRY");
	}
};

template <OMR::Model::Mode M>
struct GenDefault {
	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "DEFAULT HANDLER");
		halt(b, machine);
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenError {
	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
		OMR_TRACE();
		GEN_TRACE_MSG(b, "ERROR (UNKNOWN BYTECODE)");

		// auto b2 = b->OrphanBuilder();
		// b->Goto(b2);
		// b->AppendBuilder(b2);
		// b2->Return();

		halt(b, machine);
		return true;
	}
};

template <OMR::Model::Mode M>
struct GenHalt {
	static constexpr std::size_t INSTR_SIZE = 1;

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
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

		// auto e = b->OrphanBuilder();
		// b->Goto(e);
		// GEN_TRACE_MSG(e, "AT END");
		// // b->Goto(b->End());
		// b->AppendBuilder(e);

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

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
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

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
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

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
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

	bool operator()(OMR::Model::Builder<M>* b, Model::Machine<M>& machine) {
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

template <>
struct CallBuilder<Model::Mode::REAL> : CallBuilderBase {
	void build(Model::Machine<Model::Mode::REAL>& machine, JB::IlBuilder* b) {
		// TODO! something
	}
};

#endif // BYTECODEHANDLERS_HPP_
