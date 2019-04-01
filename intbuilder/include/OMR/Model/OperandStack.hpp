#if !defined(OMR_MODEL_VIRTOPERANDSTACK_HPP_)
#define OMR_MODEL_VIRTOPERANDSTACK_HPP_

#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Value.hpp>
#include <OMR/Model/Register.hpp>
#include <OMR/TypeTraits.hpp>

#include <IlBuilder.hpp>
#include <TypeDictionary.hpp>

#include <cstdint>
#include <vector>

namespace OMR {
namespace Model {

class VirtOperandStack {
public:
	VirtOperandStack() :
		_etype(nullptr), _ptype(nullptr), _sp(), _values() {}

	VirtOperandStack(const VirtOperandStack& other)
		: _etype(other._etype), _ptype(other._ptype), _sp(other._sp), _values(other._values) {
		fprintf(stderr, "@@@ VIRT OPERAND STACK COPY depth=%zu\n", _values.size());
	}

	void initialize(JB::IlBuilder* b, JB::IlType* etype, JB::IlValue* address) {
		JB::TypeDictionary* t = b->typeDictionary();
		_etype = etype;
		_ptype = t->PointerTo(_etype);
		_sp.initialize(b, _ptype, address);
	}

	void commit(JB::IlBuilder* b) {
		b->Call("print_s", 1, b->Const((void*)"$$$ VirtOperandStack: commit\n"));

		std::size_t n = _values.size();

		JB::IlValue* ptr = b->Sub(_sp.load(b), b->Const(8)); // Roll SP down to first slot.
		for(std::size_t i = 0; i < n; ++i) {

			auto tgt = b->IndexAt(_ptype, ptr, b->Const(0 - (std::int64_t)i));
			auto val = _values[n - i - 1];

			b->Call("print_s", 1, b->Const((void*)"$$$ VirtOperandStack: commit: store: addr="));
			b->Call("print_x", 1, tgt);
			b->Call("print_s", 1, b->Const((void*)" val="));
			b->Call("print_u", 1, val);
			b->Call("print_s", 1, b->Const((void*)"\n"));

			b->StoreAt(tgt, val);
		}
		_sp.commit(b);
	}

	void reload(JB::IlBuilder* b) {
		_sp.reload(b);

		const std::size_t nslots = _values.size();
		JB::IlValue* const ptr = b->Sub(_sp.load(b), b->Const(8)); // Roll SP down to first slot.

		for(std::size_t i = 0; i < _values.size(); ++i) {
			JB::IlValue* slotptr = b->IndexAt(_ptype, ptr, b->Const(0 - (std::int64_t)i));
			_values[nslots - i - 1] = b->LoadAt(_ptype, slotptr);
		}
	}

	void mergeInto(JB::IlBuilder* b, VirtOperandStack& dest) {

		b->Call("print_s", 1, b->Const((void*)"$$$ VirtOperandStack: merge into X\n"));

		_sp.mergeInto(b, dest._sp);

		assert(dest._values.size() == _values.size());

		for(std::size_t i = 0; i < _values.size(); ++i) {
			b->StoreOver(dest._values[i], _values[i]);
		}
	}

	/// reserve n 64bit elements on the stack. Returns a pointer to the zeroth element.
	/// In the virtual operand stack, this is "unbuffered" storage left on the stack.
	JB::IlValue* reserve64(JB::IlBuilder* b, CSize nelements) {
		JB::IlValue* start = _sp.load(b);
		_sp.store(b, b->Add(start, b->Mul(b->ConstInt64(8), nelements.toIl(b))));

		b->Call("print_s", 1, b->Const((void*)"$$$ VirtOperandStack: reserve64: nelements="));
		b->Call("print_u", 1, nelements.toIl(b));
		b->Call("print_s", 1, b->Const((void*)" new-sp="));
		b->Call("print_x", 1, _sp.load(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return start;
	}

	void pushInt64(JB::IlBuilder *b, JB::IlValue *value) {
		_values.push_back(value);
		_sp.store(b, b->Add(_sp.load(b), b->Const(8))); // TODO RWY: Using magic constant (sizeof int64)

		b->Call("print_s", 1, b->Const((void*)"$$$ VirtOperandStack: pushInt64: value="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)" new-sp="));
		b->Call("print_x", 1, _sp.load(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
	}

	JB::IlValue* peek(OMR_UNUSED JB::IlBuilder& b, CUInt offset) {
		return _values.at(offset.unpack());
	}

	JB::IlValue* popInt64(JB::IlBuilder *b) {

		_sp.store(b, b->Sub(_sp.load(b), b->Const(8))); // TODO RWY: Using magic constant sizeof int64 
		JB::IlValue* value = top(b);
		_values.pop_back();

		b->Call("print_s", 1, b->Const((void*)"$$$ VirtOperandStack: popInt64: value="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)" new-sp="));
		b->Call("print_x", 1, _sp.load(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return value;
	}

	JB::IlValue* top(JB::IlBuilder* b) {
		assert(0 < _values.size());
		return _values.at(_values.size() - 1);
	}

private:
	JB::IlType* _etype;
	JB::IlType* _ptype;
	VirtRegister _sp;
	std::vector<JB::IlValue*> _values;
};

/// grows upwards, store before increment / load after decrement.
///
class RealOperandStack {
public:
	RealOperandStack() : _sp() {}

	RealOperandStack(const RealOperandStack&) = default;

	void initialize(JB::IlBuilder* b, JB::IlType* etype, JB::IlValue* address) {
		_typedict = b->typeDictionary();
		_etype = etype;
		_ptype = _typedict->PointerTo(_etype);
		_sp.initialize(b, _ptype, address);
	}

	void commit(JB::IlBuilder* b) {
		_sp.commit(b);
	}

	void reload(JB::IlBuilder* b) {
		_sp.reload(b);
	}

	void mergeInto(JB::IlBuilder* b, RealOperandStack& dest) {
		_sp.mergeInto(b, dest._sp);
	}

	JB::IlValue* popInt64(JB::IlBuilder* b) {
		JB::IlValue* sp = b->Sub(_sp.load(b), constant(b, 8)); // TODO RWY: Using magic number (sizeof int64)
		JB::IlValue* value = b->LoadAt(_typedict->pInt64, sp);
		b->StoreAt(sp, constant(b, std::int64_t(0xdead))); // poison
		_sp.store(b, sp);

		b->Call("print_s", 1, b->Const((void*)"$$$ RealOperandStack: popInt64: value="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)" new-sp="));
		b->Call("print_x", 1, _sp.load(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
	
		return value;
	}

	void pushInt64(JB::IlBuilder* b, JB::IlValue* value) {
		JB::IlValue* sp = _sp.load(b);
		b->StoreAt(sp, value);
		_sp.store(b,
			b->ConvertTo(_ptype,
				b->Add(sp, constant(b, 8)) // TODO RWY: Using magic number (sizeof int64)
		));

		b->Call("print_s", 1, b->Const((void*)"$$$ RealOperandStack: pushInt64: value="));
		b->Call("print_u", 1, value);
		b->Call("print_s", 1, b->Const((void*)" new-sp="));
		b->Call("print_x", 1, _sp.load(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));
	}

	/// reserve n 64bit elements on the stack. Returns a pointer to the zeroth element.
	JB::IlValue* reserve64(JB::IlBuilder* b, RSize nelements) {
		JB::IlValue* start = _sp.load(b);
		_sp.store(b, b->Add(start, b->Mul(b->ConstInt64(8), nelements.unpack()))); // TODO RWY: Using magic number (sizeof int64)

		b->Call("print_s", 1, b->Const((void*)"$$$ RealOperandStack: reserve64: nelements="));
		b->Call("print_u", 1, nelements.unpack());
		b->Call("print_s", 1, b->Const((void*)" new-sp="));
		b->Call("print_x", 1, _sp.load(b));
		b->Call("print_s", 1, b->Const((void*)"\n"));

		return start;
	}

private:
	JB::TypeDictionary* _typedict;
	JB::IlType* _etype;
	JB::IlType* _ptype;
	RealRegister _sp;
};

/// Purely virtual operand stack. No side effects which can be written to the.
class PureOperandStack {
public:

};

template <Mode MODE> struct OperandStackAlias;
template <> struct OperandStackAlias<Mode::REAL> : TypeAlias<RealOperandStack> {};
template <> struct OperandStackAlias<Mode::VIRT> : TypeAlias<VirtOperandStack> {};
template <> struct OperandStackAlias<Mode::PURE> : TypeAlias<PureOperandStack> {};
template <Mode M> using OperandStack = typename OperandStackAlias<M>::Type;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_OPERANDSTACK_HPP_
