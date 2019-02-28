#if !defined(OMR_MODEL_OPERANDARRAY_HPP_)
#define OMR_MODEL_OPERANDARRAY_HPP_

#include <OMR/Model/Value.hpp>

#include <IlBuilder.hpp>
#include <IlType.hpp>
#include <TypeDictionary.hpp>

#include <vector>

namespace OMR {
namespace Model {

class RealOperandArray {
public:
	RealOperandArray(JB::IlBuilder* b, JB::IlType* type, JB::IlValue* addr, RUInt length)
		: _type(type),
		  _ptype(b->typeDictionary()->PointerTo(_type)),
		  _length(length.unpack()) {}

	void set(JB::IlBuilder* b, RUInt index, JB::IlValue* value) {
		b->StoreAt(b->IndexAt(_ptype, _addr, index.unpack()), value);
	}

	JB::IlValue* get(JB::IlBuilder* b, RUInt index) {
		return b->LoadAt(_ptype, b->IndexAt(_type, _addr, index.unpack()));
	}

	RUInt length() const { return RUInt::pack(_length); }

	void initialize() {}

	void commit(JB::IlBuilder* b) {}

	void reload(JB::IlBuilder* b) {}

private:
	JB::IlType* _type;  // element type.
	JB::IlType* _ptype; // ptr to element type.
	JB::IlValue* _addr;
	JB::IlValue* _length;
};

class VirtOperandArray {
public:
	VirtOperandArray(JB::IlBuilder* b, JB::IlType* type, JB::IlValue* addr, CUInt length)
		: _type(type),
		  _ptype(b->typeDictionary()->PointerTo(_type)),
		  _addr(addr),
		  _values(length.unpack(), nullptr) {}

	void set(JB::IlBuilder* b, CUInt index, JB::IlValue* value) {
		_values.at(index.unpack()) = value;
	}

	JB::IlValue* get(JB::IlBuilder* b, CUInt index) {
		return _values.at(index.unpack());
	}

	CUInt length() const { return CUInt::pack(_values.size()); }

	void initialize(JB::IlBuilder* b) {}

	void commit(JB::IlBuilder* b) {
		for (std::size_t i = 0; i < _values.size(); ++i) {
			JB::IlValue* value = _values.at(i);
			if (value != nullptr) {
				b->StoreAt(b->IndexAt(_ptype, _addr, b->Const((std::int64_t)i)), value);
			}
		}
	}

	void reload(JB::IlBuilder* b) {
		for (std::size_t i = 0; i < _values.size(); ++i) {
			_values.at(i) =  b->LoadAt(_ptype, b->IndexAt(_ptype, _addr, b->Const((std::int64_t)i)));
		}
	}

private:
	JB::IlType* _type;
	JB::IlType* _ptype;
	JB::IlValue* _addr;
	std::vector<JB::IlValue*> _values;
};

class PureOperandArray {
	// TODO RWY
};

template <Mode M>
struct ModalOperandArrayAlias;

template <> struct ModalOperandArrayAlias<Mode::REAL> : TypeAlias<RealOperandArray> {};
template <> struct ModalOperandArrayAlias<Mode::VIRT> : TypeAlias<VirtOperandArray> {};
template <> struct ModalOperandArrayAlias<Mode::PURE> : TypeAlias<PureOperandArray> {};

template <Mode M> using OperandArray = typename ModalOperandArrayAlias<M>::Type;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_OPERANDARRAY_HPP_
