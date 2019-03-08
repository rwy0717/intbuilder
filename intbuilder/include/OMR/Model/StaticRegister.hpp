#if !defined(OMR_MODEL_STATICREGISTER_HPP_)
#define OMR_MODEL_STATICREGISTER_HPP_

#include <OMR/Model.hpp>
#include <OMR/Model/Mode.hpp>


#include <cstdint>

namespace OMR {
namespace Model {

/*
 * @type StaticRegister A register type that is only updated with statically-determinable values.
 */
template <Mode M, typename T>
class StaticRegister;

template <typename T>
class StaticRegister<Mode::REAL, T> {
public:
	StaticRegister() : _address(nullptr), _value(nullptr) {}

	void initialize(JB::IlBuilder* b, JB::IlValue* address, RValue<T> value) {
		JB::TypeDictionary* t = b->typeDictionary();

		_vtype = t->toIlType<T>();
		_ptype = t->PointerTo(_vtype);

		_address = b->ConvertTo(_ptype, address);
		_value = value.unpack();
		store(b, value);
	}

	RValue<T> load(OMR_UNUSED JB::IlBuilder* b) const {
		return RValue<T>::pack(_value);
	};

	void store(JB::IlBuilder* b, RValue<T> value) {
		_value = value.unpack();
		b->StoreAt(_address, _value);
	}

	void commit(JB::IlBuilder* b) {}

	void reload(OMR_UNUSED JB::IlBuilder* b) {}

	/// Convert to internal representation.
	JB::IlValue* unpack() const { return _value; };

private:
	JB::IlValue* _address;
	JB::IlType* _vtype;
	JB::IlType* _ptype;
	JB::IlValue* _value;
};

template <typename T>
class StaticRegister<Mode::VIRT, T> {
public:
	StaticRegister(JB::IlValue* address) : _address(address) {}

	CValue<T> load(OMR_UNUSED JB::IlBuilder* b) const {
		return CValue<T>::pack(_value);
	}

	void store(OMR_UNUSED JB::IlBuilder* b, CValue<T> value) {
		_value = value.unpack();
	}

	void initialize(OMR_UNUSED JB::IlBuilder* b, CValue<T> value) {
		store(b, value);
	}

	void commit(JB::IlBuilder* b) {
		b->StoreAt(_address, b->Const(static_cast<typename RemoveUnsigned<T>::Type>(_value)));
	}

	void reload(OMR_UNUSED JB::IlBuilder* b) {}

	// Convert to internal representation.
	T unpack() const { return _value; }

private:
	JB::IlValue* _address;
	T _value;
};

template <typename T>
class StaticRegister<Mode::PURE, T> {
public:
	StaticRegister() {}

	void store(OMR_UNUSED JB::IlBuilder* b, CValue<T> value) {
		_value = value.unpack();
	}

	CValue<T> load(OMR_UNUSED JB::IlBuilder* b) const {
		return CValue<T>::pack(_value);
	}

	void initialize(OMR_UNUSED JB::IlBuilder* b, CValue<T> value) {
		store(b, value);
	}

	void commit(OMR_UNUSED JB::IlBuilder* b) {}

	void reload(OMR_UNUSED JB::IlBuilder* b) {}

	T unpack() const { return _value; }

private:
	T _value;
};

template <typename T>
using RealStaticRegister = StaticRegister<Mode::REAL, T>;

template <typename T>
using VirtStaticRegister = StaticRegister<Mode::VIRT, T>;

template <typename T>
using PureStaticRegister = StaticRegister<Mode::PURE, T>;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_STATICREGISTER_HPP_
