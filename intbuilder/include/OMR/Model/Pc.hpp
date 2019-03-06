#if !defined(OMR_MODEL_PC_HPP_)
#define OMR_MODEL_PC_HPP_

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

class RealPc {
public:
	RealPc() : _pc(), _fp() {}

	/// Set up the function pointer.
	void initialize(JB::IlBuilder* b, JB::IlValue* pcAddr, JB::IlValue* fpAddr, RUIntPtr fn) {
		_pc.initialize(b, pcAddr, fn);
		_fp.initialize(b, fpAddr, fn);
	}

	RUInt immediateUInt(JB::IlBuilder* b, RUInt offset) {
		return RUInt::pack(read<unsigned int>(b, offset.unpack()));
	}

	RUInt immediateUInt(JB::IlBuilder* b) {
		return immediateUInt(b, RUInt(b, 0));
	}

	RInt immediateInt(JB::IlBuilder* b, RUInt offset) {
		return RInt::pack(read<int>(b, offset.unpack()));
	}

	RInt immediateInt(JB::IlBuilder* b) {
		return immediateInt(b, RUInt(b, 0));
	}

	void next(JB::IlBuilder* b, RUInt offset) {
		_pc.store(b, RUIntPtr::pack(b->Add(_pc.unpack(), offset.unpack())));
	}

	void halt(JB::MethodBuilder* b) {
		b->Return();
	}

	void halt(JB::MethodBuilder* b, JB::IlValue* result) {
		b->Return(result);
	}

	RUIntPtr offset(JB::IlBuilder* b) {
		return RUIntPtr::pack(b->Sub(_pc.unpack(), _fp.unpack()));
	}

	RUIntPtr function(JB::IlBuilder* b) { return _fp.load(b); }

	RUIntPtr load(JB::IlBuilder* b) { return _pc.load(b); }

	void commit(JB::IlBuilder* b) {}

	void reload(JB::IlBuilder* b) {}

private:
	template <typename T>
	JB::IlValue* read(JB::IlBuilder* b, JB::IlValue* offset = 0) {
		return b->LoadAt(b->typeDictionary()->toIlType<T>(),
					b->IndexAt(b->Int8, _pc.unpack(), offset));
	}

	JB::IlType* _type;
	JB::IlType* _ptype;
	RealStaticRegister<std::uintptr_t> _pc;
	RealStaticRegister<std::uintptr_t> _fp;
};

/// In the virtual program counter, the program is treated as a constant.
/// immediates and program decoding is collapsed to real constant values.
class VirtPc {
public:
	VirtPc(JB::IlValue* pcAddr, JB::IlValue* fpAddr)
		: _pc(pcAddr), _fp(fpAddr) {}

	CUInt immediateUInt(JB::IlBuilder* b, CUInt offset) {
		return CUInt(b, read<unsigned int>(offset.unpack()));
	}

	CUInt immediateUInt(JB::IlBuilder* b) {
		return immediateUInt(b, CUInt(b, 0));
	}

	CInt immediateInt(JB::IlBuilder* b, CUInt offset) {
		return CInt(b, read<int>(offset.unpack()));
	}

	CInt immediateInt(JB::IlBuilder* b) {
		return immediateInt(b, CUInt(b, 0));
	}

	void next(JB::IlBuilder* b, CUInt offset) {
		_pc.store(b, CUIntPtr::pack(_pc.unpack() + offset.unpack()));
		// todo: dispatch to next bytecode builder?
	}

	CUIntPtr offset(JB::IlBuilder* b) const {
		return sub(b, _pc.load(b), _fp.load(b));
	}

	CUIntPtr function(JB::IlBuilder* b) { return _fp.load(b); }

	CUIntPtr load(JB::IlBuilder* b) { return _pc.load(b); }

	void initialize(JB::IlBuilder* b, CUIntPtr function) {
		_pc.initialize(b, function);
		_fp.initialize(b, function);
	}

	void commit(JB::IlBuilder* b) {
		_pc.commit(b);
		_fp.commit(b);
	}

	void reload(JB::IlBuilder* b) {}

	JB::BytecodeBuilderTable* bytecodeBuilders() { return &_bytecodeBuilders; }


private:
	template <typename T>
	T read(std::size_t offset = 0) {
		return *reinterpret_cast<T*>(_pc.unpack() + offset);
	}

	VirtStaticRegister<std::uintptr_t> _pc; // program counter
	VirtStaticRegister<std::uintptr_t> _fp; // function pointer

	JB::BytecodeBuilder* _currentBytecodeBuilder;
	JB::BytecodeBuilderTable _bytecodeBuilders;
};

template <Mode M> struct ModalPcAlias;
template <> struct ModalPcAlias<Mode::REAL> : TypeAlias<RealPc> {};
template <> struct ModalPcAlias<Mode::VIRT> : TypeAlias<VirtPc> {};

template <Mode M> using Pc = typename ModalPcAlias<M>::Type;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_PC_HPP_
