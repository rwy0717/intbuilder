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
	RealOperandArray() :
		_type(nullptr), _ptype(nullptr), _addr(nullptr), _length(nullptr) {}

	void initialize(JB::IlBuilder* b, JB::IlType* type, JB::IlValue* addr, RSize length) {
		_type = type;
		_ptype = b->typeDictionary()->PointerTo(type);
		_addr = addr;
		_length = length.unpack();
	}

	void set(JB::IlBuilder* b, RSize index, JB::IlValue* value) {
		b->StoreAt(b->IndexAt(_ptype, _addr, index.unpack()), value);
	}

	JB::IlValue* get(JB::IlBuilder* b, RSize index) {
		return b->LoadAt(_ptype, b->IndexAt(_ptype, _addr, index.unpack()));
	}

	RSize length() const { return RSize::pack(_length); }

	void commit(JB::IlBuilder* b) {}

	void reload(JB::IlBuilder* b) {}

	void mergeInto(JB::IlBuilder* b, RealOperandArray& dest) {
		b->StoreOver(dest._addr, _addr);
		b->StoreOver(dest._length, _length);
	}

private:
	JB::IlType* _type;  // element type.
	JB::IlType* _ptype; // ptr to element type.
	JB::IlValue* _addr;
	JB::IlValue* _length;
};

class VirtOperandArray {
public:
	VirtOperandArray() : _type(nullptr), _ptype(nullptr), _addr(nullptr), _values() {}

	void initialize(JB::IlBuilder* b, JB::IlType* type, JB::IlValue* addr, CSize length) {
		_type = type;
		_ptype = b->typeDictionary()->PointerTo(_type);
		_addr = addr;
		_values.assign(length.unpack(), nullptr);
	}

	void set(JB::IlBuilder* b, CUInt index, JB::IlValue* value) {
		_values.at(index.unpack()) = value;
	}

	JB::IlValue* get(JB::IlBuilder* b, CUInt index) {
		return _values.at(index.unpack());
	}

	CUInt length() const { return CUInt::pack(_values.size()); }

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
