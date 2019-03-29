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

/// pc
/// startPc
class RealPc {
public:
	RealPc() : _ptype(nullptr), _address(nullptr), _base(nullptr) {}

	RealPc(const RealPc& other) = default;

	RealPc(RealPc&& other) = default;

	/// Set up the function pointer.
	void initialize(JB::IlBuilder* b, JB::IlValue* address, RPtr<std::uint8_t> value) {
		_ptype = b->typeDictionary()->toIlType<std::uint8_t**>();
		_address = address;
		_base = value.unpack();
		b->StoreAt(_address, _base);
		b->Call("print_s", 1, b->Const((void*)"$$$ RealPc initialize value="));
		b->Call("print_x", 1, xload(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
	}

	RPtr<std::uint8_t> load(RBuilder* b) const {
		return xload(b);
	}

	/// Load from outside a bytecode handler.
	RPtr<std::uint8_t> xload(JB::IlBuilder* b) const {
		JB::IlValue* value = b->LoadAt(_ptype, _address);
		b->Call("print_s", 1, b->Const((void*)"Pc loading: value="));
		b->Call("print_x", 1, value);
		b->Call("print_s", 1, b->Const((void*)"\n"));
		return RPtr<std::uint8_t>::pack(value);
	}

	void commit(JB::IlBuilder* b) {}

	void reload(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, RealPc& dest) {}

	/// @group Unsafe / non-generic functionality
	/// @{

	JB::IlValue* base() const { return _base; }

	JB:: IlValue* unpack(RBuilder* b) const { return load(b).unpack(); }

	JB::IlValue* offset(RBuilder* b) const {
		JB::IlValue* value =
			 b->Sub(
				b->ConvertTo(b->Word, unpack(b)),
				b->ConvertTo(b->Word, _base));

		b->Call("print_s", 1, b->Const((void*)"Pc offset: value="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return value;
	}

	/// @}
	///

private:
	JB::IlType* _ptype;
	JB::IlValue* _address;
	JB::IlValue* _base;
};

/// Read-only access to the PC (aka instruction-pointer) register.
/// For this API to work, the bytecode index must be exactly each bytecode's offset into the bytecode stream.
/// Down the road, the mapping of bytecode-index to offset may be controlled by users.
class VirtPc {
public:
	VirtPc() : _address(nullptr), _base(nullptr) {}

	void initialize(JB::IlBuilder* b, JB::IlValue* address, CPtr<std::uint8_t> value) {
		b->Call("print_s", 1, b->Const((void*)"$$$ VirtPc initialize value="));
		b->Call("print_x", 1, value.toIl(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
		_address = address;
		_base = value.unpack();
	}

	CPtr<std::uint8_t> load(CBuilder* b) const {
		b->Call("print_s", 1, b->Const((void*)"$$$ VirtPc: load value="));
		b->Call("print_x", 1, constant(b, unpack(b)));
		b->Call("print_s", 1, b->Const((void*)"\n"));
		return CPtr<std::uint8_t>::pack(unpack(b));
	}

	void commit(CBuilder* b) {
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
	std::uint8_t* _base;
};

template <Mode M> struct PcAlias;
// template <> struct PcAlias<Mode::REAL> : TypeAlias<RealPc> {};
template <> struct PcAlias<Mode::VIRT> : TypeAlias<VirtPc> {};

template <Mode M> using Pc = typename PcAlias<M>::Type;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_PC_HPP_
