#if !defined(OMR_MODEL_PC_HPP_)
#define OMR_MODEL_PC_HPP_

#include <OMR/Model/ControlFlow.hpp>
#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Value.hpp>
#include <OMR/Model/StaticRegister.hpp>
#include <OMR/Model.hpp>
#include <OMR/TypeTraits.hpp>
#include <IlBuilder.hpp>
#include <IlType.hpp>
#include <BytecodeBuilderTable.hpp>
#include <MethodBuilder.hpp>

namespace OMR {
namespace Model {

/// V
/// pc
/// startPc
class RealPc {
public:
	RealPc(OMR_UNUSED MethodBuilderData* data) : _pcReg(), _startPcReg() {}

	RealPc(const RealPc& other) = default;

	RealPc(RealPc&& other) = default;

	/// Set up the function pointer.
	void initialize(JB::IlBuilder* b, JB::IlValue* pcAddr, JB::IlValue* startPcAddr, RPtr<std::uint8_t> value) {
		b->Call("print_s", 1, b->Const((void*)"Pc initial value="));
		b->Call("print_x", 1, value.unpack());
		b->Call("print_s", 1, b->Const((void*)"\n"));

		_pcReg.initialize(b, pcAddr, value);
		_startPcReg.initialize(b, startPcAddr, value);

		b->Call("print_s", 1, b->Const((void*)"Pc value immediately reloaded value="));
		b->Call("print_x", 1, _pcReg.unpack());
		b->Call("print_s", 1, b->Const((void*)"\n"));	
	}

	RUInt64 immediateUInt64(JB::IlBuilder* b, RSize offset) {
		return RUInt64::pack(read<std::uint64_t>(b, offset.unpack()));
	}

	RUInt64 immediateUInt64(JB::IlBuilder* b) {
		return immediateUInt64(b, RSize(b, 0));
	}

	RInt64 immediateInt64(JB::IlBuilder* b, RSize offset) {
		return RInt64::pack(read<std::int64_t>(b, offset.unpack()));
	}

	RInt64 immediateInt64(JB::IlBuilder* b) {
		return immediateInt64(b, RSize(b, 0));
	}

	RSize immediateSize(JB::IlBuilder* b, RSize offset) {
		return RSize::pack(read<std::size_t>(b, offset.unpack()));
	}

	RSize immediateSize(JB::IlBuilder* b) {
		return immediateSize(b, RSize(b, 0));
	}

	/// @group Inter-bytecode Control Flow
	/// @{

	void next(JB::IlBuilder* b, RSize offset) {
		_pcReg.store(b, RPtr<std::uint8_t>::pack(
			b->Add(
				_pcReg.unpack(),
				offset.unpack()
		)));
		b->Call("print_s", 1, b->Const((void*)"Pc updated: value="));
		b->Call("print_u", 1, _pcReg.unpack());
		b->Call("print_s", 1, b->Const((void*)"\n"));
	}

	void halt(JB::IlBuilder* b) {
		b->Return();
	}

	void halt(JB::MethodBuilder* b, JB::IlValue* result) {
		b->Return(result);
	}

	/// @}
	///

	/// @group State Queries
	/// @{

	RUIntPtr offset(JB::IlBuilder* b) {
		return RUIntPtr::pack(
			b->Sub(
				_pcReg.unpack(),
				_startPcReg.unpack()
		));
	}

	RPtr<std::uint8_t> start(JB::IlBuilder* b) { return _startPcReg.load(b); }

	RPtr<std::uint8_t> load(JB::IlBuilder* b) {

		b->Call("print_s", 1, b->Const((void*)"Pc loading: value="));
		b->Call("print_u", 1, _pcReg.unpack());
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return _pcReg.load(b);

	}

	/// @}
	///

	void commit(JB::IlBuilder* b) {
		_pcReg.commit(b);
		_startPcReg.commit(b);
	}

	void reload(JB::IlBuilder* b) {
		_pcReg.reload(b);
		_startPcReg.reload(b);
	}

	void mergeInto(JB::IlBuilder* b, RealPc& dest) {
		_pcReg.mergeInto(b, dest._pcReg);
		_startPcReg.mergeInto(b, dest._startPcReg);
	}

private:
	template <typename T>
	JB::IlValue* read(JB::IlBuilder* b, JB::IlValue* offset = 0) {
		JB::TypeDictionary* t = b->typeDictionary();

		JB::IlValue* addr = b->IndexAt(t->pInt8, _pcReg.load(b).toIl(b), offset);
		JB::IlValue* value = b->LoadAt(t->PointerTo(t->toIlType<T>()), addr);

		b->Call("print_s", 1, b->Const((void*)"PC READ: pc="));
		b->Call("print_x", 1, _pcReg.load(b).toIl(b));
		b->Call("print_s", 1, b->Const((void*)" offset="));
		b->Call("print_x", 1, offset);
		b->Call("print_s", 1, b->Const((void*)" addr="));
		b->Call("print_x", 1, addr);
		b->Call("print_s", 1, b->Const((void*)" val="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return value;
	}

	// JB::IlType* _type;
	// JB::IlType* _ptype;
	RealStaticRegister<std::uint8_t*> _pcReg;
	RealStaticRegister<std::uint8_t*> _startPcReg;
};

/// In the virtual program counter, the program is treated as a constant.
/// immediates and program decoding is collapsed to real constant values.
class VirtPc {
public:
	VirtPc(MethodBuilderData* mbdata) : _pcReg(), _startPcReg(), _controlFlow(mbdata) {}

	CUInt64 immediateUInt64(JB::IlBuilder* b, CSize offset) {
		return CUInt64(b, read<std::uint64_t>(offset.unpack()));
	}

	CUInt64 immediateUInt64(JB::IlBuilder* b) {
		return immediateUInt64(b, CSize(b, 0));
	}

	CInt64 immediateInt64(JB::IlBuilder* b, CSize offset) {
		return CInt64(b, read<std::int64_t>(offset.unpack()));
	}

	CInt64 immediateInt64(JB::IlBuilder* b) {
	return immediateInt64(b, CSize(b, 0));
	}

	CSize immediateSize(JB::IlBuilder* b, CSize offset) {
		return CSize(b, read<std::int64_t>(offset.unpack()));
	}

	void next(JB::IlBuilder* b, CSize offset) {
		JB::BytecodeBuilder* bb = static_cast<JB::BytecodeBuilder*>(b);
		std::size_t index = _pcReg.unpack() - _startPcReg.unpack() + offset.unpack();

		b->Call("print_s", 1, b->Const((void*)"VirtPc: next offset=\n"));
		b->Call("print_u", 1, offset.toIl(b));
		b->Call("print_s", 1, b->Const((void*)" index=\n"));
		b->Call("print_u", 1, b->Const((std::int64_t)index));
	
		_pcReg.store(b, CPtr<std::uint8_t>::pack(_pcReg.unpack() + offset.unpack()));
		_controlFlow.next(b, index);
		
	}

	CUIntPtr offset(JB::IlBuilder* b) const {
		return CUIntPtr::pack(
			_pcReg.unpack() - _startPcReg.unpack()
		);
	}

	CPtr<std::uint8_t> start(JB::IlBuilder* b) { return _startPcReg.load(b); }

	CPtr<std::uint8_t> load(JB::IlBuilder* b) { return _pcReg.load(b); }

	void initialize(JB::IlBuilder* b, JB::IlValue* pcAddr, JB::IlValue* startPcAddr, CPtr<std::uint8_t> value) {

		b->Call("print_s", 1, b->Const((void*)"Pc initial value="));
		b->Call("print_x", 1, value.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		_pcReg.initialize(b, pcAddr, value);
		_startPcReg.initialize(b, startPcAddr, value);
	}

	void commit(JB::IlBuilder* b) {
		_pcReg.commit(b);
		_startPcReg.commit(b);
	}

	void reload(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, VirtPc& dest) {
		_pcReg.mergeInto(b, dest._pcReg);
		_startPcReg.mergeInto(b, dest._startPcReg);
	}

private:
	template <typename T>
	T read(std::size_t offset = 0) {
		return *reinterpret_cast<T*>(_pcReg.unpack() + offset);
	}

	VirtStaticRegister<std::uint8_t*> _pcReg;      // program counter
	VirtStaticRegister<std::uint8_t*> _startPcReg; // function pointer
	VirtControlFlow _controlFlow;
};

template <Mode M> struct PcAlias;
template <> struct PcAlias<Mode::REAL> : TypeAlias<RealPc> {};
template <> struct PcAlias<Mode::VIRT> : TypeAlias<VirtPc> {};

template <Mode M> using Pc = typename PcAlias<M>::Type;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_PC_HPP_
