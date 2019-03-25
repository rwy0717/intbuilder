#if !defined(OMR_MODEL_PC_HPP_)
#define OMR_MODEL_PC_HPP_

#include <OMR/Model/Builder.hpp>
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
	RealPc() : _address(nullptr), _base(nullptr), _value(nullptr) {}

	RealPc(const RealPc& other) = default;

	RealPc(RealPc&& other) = default;

	/// Set up the function pointer.
	void initialize(JB::IlBuilder* b, JB::IlValue* pcAddress, JB::IlValue* startPcAddress, RPtr<std::uint8_t> value) {

		JB::TypeDictionary* t = b->typeDictionary();
		JB::IlType* type = t->toIlType<std::uint8_t*>();

		b->Call("print_s", 1, b->Const((void*)"$$$ RealPc initialize value="));
		b->Call("print_x", 1, value.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		_pc.initialize(b, type, pcAddress);
		_startpc.initialize(b, type, startPcAddress);
	}

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
	RealRegister _pc;
	RealRegister _startpc;
};

/// Read-only access to the PC (aka instruction-pointer) register.
/// For this API to work, the bytecode index must be exactly each bytecode's offset into the bytecode stream.
/// Down the road, the mapping of bytecode-index to offset may be controlled by users.
class VirtPc {
public:
	VirtPc() : _address(nullptr), _base(nullptr) {}

	CPtr<std::uint8_t> load(JB::BytecodeBuilder* b) const {
		b->Call("print_s", 1, b->Const((void*)"$$$ VirtPc: load value="));
		b->Call("print_x", 1, constant(b, unpack(b)));
		b->Call("print_s", 1, b->Const((void*)"\n"));
		return CPtr<std::uint8_t>::pack(unpack(b));
	}

	void initialize(JB::IlBuilder* b, JB::IlValue* address, CPtr<std::uint8_t> value) {
		b->Call("print_s", 1, b->Const((void*)"$$$ VirtPc initialize value="));
		b->Call("print_x", 1, value.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
		_address = address;
		_base = value.unpack();
	}

	void commit(JB::BytecodeBuilder* b) const {
		b->Call("print_s", 1, b->Const((void*)"$$$ VirtPc: commit: value="));
		b->Call("print_x", 1, load(b).toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
		b->StoreAt(_address, constant(b, unpack(b)));
	}

	void reload(OMR_UNUSED JB::IlBuilder* b) {}

	void mergeInto(OMR_UNUSED JB::IlBuilder* b, VirtPc& dest) {}

	/// @group Unsafe / non-generic functionality.
	/// @{

	/// Returns the initial value of the PC, the address of the first instruction. Internal API.
	std::uint8_t* base() const { return _base; }

	/// Obtain the compile-time value of the PC. Internal API.
	/// The PC is derived from the current bytecode builder.
	std::uint8_t* unpack(CBuilder* b) const { return _base + offset(b); }

	std::size_t offset(CBuilder* b) const { return b->bcIndex(); }

	/// @}
	///

private:
	JB::IlValue* _address;
	JB::IlValue* _pcStartAddr;
	std::uint8_t* _base;
};

template <Mode M> struct PcAlias;
// template <> struct PcAlias<Mode::REAL> : TypeAlias<RealPc> {};
template <> struct PcAlias<Mode::VIRT> : TypeAlias<VirtPc> {};

template <Mode M> using Pc = typename PcAlias<M>::Type;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_PC_HPP_
