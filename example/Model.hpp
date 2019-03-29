#if !defined(MODEL_HPP_)
#define MODEL_HPP_

#include "Interpreter.hpp"

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

#include <cstdint>
#include <cassert>
#include <memory>

namespace Model {

namespace JB = ::OMR::JitBuilder;
using namespace OMR::Model::ValueTypes;
using OMR::Model::Mode;
using OMR::Model::Builder;
using OMR::Model::CBuilder;
using OMR::Model::RBuilder;

/// Current function metadata.
/// Wrapper for accessing Func structures through the machine model.
template <Mode M>
class Func;

template <>
class Func<Mode::VIRT> {
public:
	Func() : _function(nullptr) {}

	void initialize(OMR_UNUSED JB::IlBuilder* b, CPtr<::Func> function) {
		_function = function.unpack();
	}

	CSize nlocals(OMR_UNUSED JB::IlBuilder* b) const {
		return OMR::Model::CSize::pack(_function->nlocals);
	}

	CSize nparams(OMR_UNUSED JB::IlBuilder* b) const {
		return OMR::Model::CSize::pack(_function->nparams);
	}

	CPtr<std::uint8_t> body(OMR_UNUSED JB::IlBuilder* b) const {
		return CPtr<std::uint8_t>::pack(
			_function->body
		);
	}

	CValue<CompiledFn> cbody(OMR_UNUSED JB::IlBuilder* b) const {
		return CValue<CompiledFn>::pack(_function->cbody);
	}

	::Func* unpack() const { return _function; }

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, Func<Mode::VIRT>& dest) {}

private:
	::Func* _function;
};

template <>
class Func<Mode::REAL> {
public:
	Func() : _address(nullptr) {}

	void initialize(OMR_UNUSED JB::IlBuilder* b, RPtr<::Func> function) {
		_address = function.unpack();
	}

	RValue<CompiledFn> cbody(JB::IlBuilder* b) const {
		return RValue<CompiledFn>::pack(
			b->LoadIndirect("Func", "cbody", _address)
		);
	}

	RSize nparams(JB::IlBuilder* b) const {
		return RSize::pack(
			b->LoadIndirect("Func", "nparams", _address)
		);
	}

	RSize nlocals(JB::IlBuilder* b) const {
		return RSize::pack(
			b->LoadIndirect("Func", "nlocals", _address)
		);
	}

	RPtr<std::uint8_t> body(JB::IlBuilder* b) const {
		return RPtr<std::uint8_t>::pack(
			b->StructFieldInstanceAddress("Func", "body", _address)
		);
	}

	JB::IlValue* unpack() const { return _address; }

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, Func<Mode::REAL>& dest) {}

	void reload(JB::IlBuilder* b) {}

private:
	JB::IlValue* _address;
};

using RealFunc = Func<Mode::REAL>;
using VirtFunc = Func<Mode::VIRT>;

/// Interface for accessing static data about the current instruction.
template <Mode M>
class Instruction;

template <>
class Instruction<Mode::VIRT> {
public:
	Instruction() : _func() {}

	void initialize(JB::IlBuilder* b, JB::IlValue* pc, CPtr<::Func> func) {
		_func.initialize(b, func);
		_pc.initialize(b, pc, _func.body(b));
	}

	/// Get the address of the current function by loading from the PC.
	CPtr<std::uint8_t> address(Model::CBuilder* b) const {
		return _pc.load(b);
	} 

	const VirtFunc& func() const { return _func; }

	const OMR::Model::VirtPc& pc() const { return _pc; }

	/// The current bytecode index.
	CSize index(JB::BytecodeBuilder* b) const {
		return CSize::pack(b->bcIndex());
	}

	CUInt64 immediateUInt64(Model::CBuilder* b, CSize offset) {
		return CUInt64(b, read<std::uint64_t>(b, offset.unpack()));
	}

	CUInt64 immediateUInt64(Model::CBuilder* b) {
		return immediateUInt64(b, CSize(b, 0));
	}

	CInt64 immediateInt64(Model::CBuilder* b, CSize offset) {
		return CInt64(b, read<std::int64_t>(b, offset.unpack()));
	}

	CInt64 immediateInt64(Model::CBuilder* b) {
		return immediateInt64(b, CSize(b, 0));
	}

	CSize immediateSize(Model::CBuilder* b, CSize offset) {
		return CSize(b, read<std::int64_t>(b, offset.unpack()));
	}

	template <typename T>
	CValue<T> immediate(Model::CBuilder* b, CSize offset) {
		return CValue<T>::pack(read<T>(b, offset.unpack()));
	}

	template <typename T>
	CValue<T> immediate(Model::CBuilder* b) {
		return CValue<T>::pack(read<T>(b));
	}

	void commit(JB::IlBuilder* b) {}

	void reload(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, Instruction<Mode::VIRT>& dest) {}

private:
	template <typename T>
	T read(CBuilder* b, std::size_t offset = 0) {
		return *reinterpret_cast<T*>(_pc.unpack(b) + offset);
	}

	VirtFunc _func;
	OMR::Model::VirtPc _pc;
};

template <typename T>
CValue<T> immediate(Model::CBuilder* b, const Instruction<Mode::VIRT>& instruction, std::size_t offset = 0) {
	return CValue<T>::pack(instruction.immediate<T>(b, offset));
}

template <typename T>
CValue<T> immediate(Model::CBuilder* b, const Instruction<Mode::VIRT>& instruction, CSize offset) {
	return immediate<T>(b, instruction, offset.unpack());
}

template <>
class Instruction<Mode::REAL> {
public:
	static constexpr Mode M = Mode::REAL;

	Instruction() {}

	void initialize(JB::IlBuilder* b, JB::IlValue* pc, RPtr<::Func> func) {
		_func.initialize(b, func);
		_pc.initialize(b, pc, _func.body(b));
	}

	RPtr<std::uint8_t> address(RBuilder* b) const {
		return _pc.load(b);
	}

	RPtr<std::uint8_t> xaddress(JB::IlBuilder* b) const {
		return _pc.xload(b);
	}

	const RealFunc& func() const { return _func; }

	const OMR::Model::RealPc& pc() const { return _pc; }

	RSize index(RBuilder* b) const {
		return RSize::pack(_pc.offset(b));
	}

	RUInt64 immediateUInt64(RBuilder* b, RSize offset) {
		return RUInt64::pack(read<std::uint64_t>(b, offset.unpack()));
	}

	RUInt64 immediateUInt64(RBuilder* b) {
		return immediateUInt64(b, RSize(b, 0));
	}

	RInt64 immediateInt64(RBuilder* b, RSize offset) {
		return RInt64::pack(read<std::int64_t>(b, offset.unpack()));
	}

	RInt64 immediateInt64(RBuilder* b) {
		return immediateInt64(b, RSize(b, 0));
	}

	RSize immediateSize(RBuilder* b, RSize offset) {
		return RSize::pack(read<std::size_t>(b, offset.unpack()));
	}

	RSize immediateSize(RBuilder* b) {
		return immediateSize(b, RSize(b, 0));
	}

	void commit(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, Instruction<Mode::REAL>& i) {}

	void reload(JB::IlBuilder* b) {}

private:
	template <typename T>
	JB::IlValue* read(RBuilder* b, JB::IlValue* offset = 0) {
		JB::TypeDictionary* t = b->typeDictionary();

		JB::IlValue* addr = b->IndexAt(t->pInt8, _pc.load(b).toIl(b), offset);
		JB::IlValue* value = b->LoadAt(t->PointerTo(t->toIlType<T>()), addr);

		b->Call("print_s", 1, b->Const((void*)"PC READ: pc="));
		b->Call("print_x", 1, _pc.load(b).toIl(b));
		b->Call("print_s", 1, b->Const((void*)" offset="));
		b->Call("print_x", 1, offset);
		b->Call("print_s", 1, b->Const((void*)" addr="));
		b->Call("print_x", 1, addr);
		b->Call("print_s", 1, b->Const((void*)" val="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return value;
	}

	RealFunc _func;
	OMR::Model::RealPc _pc;
};

template <Mode M>
class Machine final : public JB::VirtualMachineState {
public:
	class Factory {
	public:
		void setInterpreter(JB::IlValue* interpreter) { _interpreter = interpreter; }

		void setFunction(Ptr<M, ::Func> function) { _function = function; }

		Machine<M>* create(JB::IlBuilder* b, OMR::Model::FunctionData<M>& data) {

			JB::TypeDictionary* t = b->typeDictionary();

			assert(_interpreter != nullptr);
	
			Model::Machine<M>* machine = new Model::Machine<M>(data);

			JB::IlValue* pcAddr      = b->StructFieldInstanceAddress("Interpreter", "_pc",      _interpreter);
			JB::IlValue* spAddr      = b->StructFieldInstanceAddress("Interpreter", "_sp",      _interpreter);
			JB::IlValue* fpAddr      = b->StructFieldInstanceAddress("Interpreter", "_fp",      _interpreter);
			JB::IlValue* startPcAddr = b->StructFieldInstanceAddress("Interpreter", "_startpc", _interpreter);

			machine->instruction.initialize(b, pcAddr, _function);

			machine->stack.initialize(b, t->Int64, spAddr);

			machine->control.initialize(b, pcAddr);
	
			OMR::Model::Size<M> nlocals = machine->instruction.func().nlocals(b);
			JB::IlValue* localsAddr = machine->stack.reserve64(b, nlocals);
			machine->locals.initialize(b, t->Int64, localsAddr, nlocals);

			return machine;
		}

	private:
		JB::IlValue* _interpreter = nullptr;
		Ptr<M, ::Func> _function;
		JB::BytecodeBuilderTable* _builders = nullptr;
	};

	Machine(OMR::Model::FunctionData<M>& data) : control(data) {}

	Machine(const Machine&) = default;

	Machine(Machine&&) = default;

	void commit(JB::IlBuilder* b) {
		instruction.commit(b);
		stack.commit(b);
		locals.commit(b);
	}

	void mergeInto(JB::IlBuilder* b, Machine<M>& dest) {
		instruction.mergeInto(b, dest.instruction);
		stack.mergeInto(b, dest.stack);
		locals.mergeInto(b, dest.locals);
	}

	void reload(JB::IlBuilder* b) {
		instruction.reload(b);
		stack.reload(b);
		locals.reload(b);
	}

	/// @group VirtualMachineState implementation
	/// @{

	virtual void Commit(JB::IlBuilder* b) override final {
		commit(b);
	}

	virtual void MergeInto(JB::VirtualMachineState* dest, JB::IlBuilder* b) override final {
		fprintf(stderr, "@@@ Machine %p MergeInto %p\n", this, dest);
		mergeInto(b, *reinterpret_cast<Model::Machine<M>*>(dest));
	}

	virtual JB::VirtualMachineState* MakeCopy() override final {
		auto copy = new Model::Machine<M>(*this);
		fprintf(stderr, "@@@ Machine this=%p MakeCopy copy=%p\n", this, copy);
		return copy;
	}

	/// @}
	///

	Instruction<M> instruction;
	OMR::Model::OperandStack<M> stack;
	OMR::Model::OperandArray<M> locals;
	OMR::Model::ControlFlow<M> control;

private:
	friend class Factory;

	Machine() {}
};

using RealMachine = Machine<Mode::REAL>;
using VirtMachine = Machine<Mode::VIRT>;

///
/// Runtime control flow operations.
///

void halt(Model::RBuilder* b, RealMachine& machine) {
	b->Call("print_s", 1, b->Const((void*)"$$$ machine halt\n"));
	// machine.commit(b);
	b->Return();
}

/// relative fallthrough.
void next(Model::RBuilder* b, RealMachine& machine, RSize offset) {
	JB::IlValue* off = offset.unpack();
	JB::IlValue* index = machine.instruction.index(b).unpack();
	JB::IlValue* target = b->Add(index, off);

	b->Call("print_s", 1, b->Const((void*)"$$$ machine next: offset="));
	b->Call("print_u", 1, off);
	b->Call("print_s", 1, b->Const((void*)" target-index="));
	b->Call("print_u", 1, target);
	b->Call("print_s", 1, b->Const((void*)"\n"));

	machine.control.next(b, target);
}

/// Relative conditional if, signed offset.
void ifCmpNotEqualZero(Model::RBuilder* b, RealMachine& machine, JB::IlValue* cond, RInt64 offset) {

	JB::IlValue* off = offset.unpack();
	JB::IlValue* index = machine.instruction.index(b).unpack();
	JB::IlValue* pc = machine.instruction.address(b).unpack();
	JB::IlValue* target = b->Add(index, off);
	JB::IlValue* targetpc = b->Add(pc, off);

	b->Call("print_s", 1, b->Const((void*)"$$$ machine ifCmpNotEqualZero offset="));
	b->Call("print_u", 1, off);
	b->Call("print_s", 1, b->Const((void*)" target-pc="));
	b->Call("print_u", 1, targetpc);
	b->Call("print_s", 1, b->Const((void*)" target-index="));
	b->Call("print_u", 1, target);
	b->Call("print_s", 1, b->Const((void* )"\n"));

	// _pcReg.store(b, CPtr<std::uint8_t>::pack(targetPc));
	machine.control.IfCmpNotEqualZero(b, cond, target);
}

///
/// Compile-time control flow operations
///

void halt(Model::CBuilder* b, VirtMachine& machine) {
	b->Call("print_s", 1, b->Const((void*)"$$$ machine halt\n"));
	machine.commit(b);
	b->Return();
}

/// relative fallthrough.
void next(Model::CBuilder* b, VirtMachine& machine, CSize offset) {
	std::intptr_t off = offset.unpack();
	std::size_t index = machine.instruction.index(b).unpack();
	std::size_t target = index + off;

	b->Call("print_s", 1, b->Const((void*)"$$$ machine next: offset="));
	b->Call("print_u", 1, offset.toIl(b));
	b->Call("print_s", 1, b->Const((void*)" target-index="));
	b->Call("print_u", 1, b->Const((std::int64_t)target));
	b->Call("print_s", 1, b->Const((void*)"\n"));

	machine.control.next(b, target);
}

/// Relative conditional if, signed offset.
void ifCmpNotEqualZero(Model::CBuilder* b, VirtMachine& machine, JB::IlValue* cond, CInt64 offset) {

	std::intptr_t off = offset.unpack();
	std::size_t index = machine.instruction.index(b).unpack();
	std::uint8_t* pc = machine.instruction.address(b).unpack();
	std::size_t target = index + off;
	std::uint8_t* targetpc = pc + off;

	b->Call("print_s", 1, b->Const((void*)"$$$ machine ifCmpNotEqualZero offset="));
	b->Call("print_u", 1, b->Const((std::int64_t)off));
	b->Call("print_s", 1, b->Const((void*)" target-pc="));
	b->Call("print_u", 1, b->Const((std::int64_t)targetpc));
	b->Call("print_s", 1, b->Const((void*)" target-index="));
	b->Call("print_u", 1, b->Const((std::int64_t)target));
	b->Call("print_s", 1, b->Const((void* )"\n"));

	// _pcReg.store(b, CPtr<std::uint8_t>::pack(targetPc));
	machine.control.IfCmpNotEqualZero(b, cond, target);
}

}  // namespace Model

#endif // MODEL_HPP_
