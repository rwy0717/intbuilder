#if !defined(OMR_MODEL_BUILDER_HPP_)
#define OMR_MODEL_BUILDER_HPP_

#include <OMR/Model.hpp>
#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Value.hpp>

#include <RBuilder.hpp>
#include <CBuilder.hpp>

#include <TypeDictionary.hpp>
#include <IlBuilder.hpp>
#include <MethodBuilder.hpp>
#include <BytecodeBuilder.hpp>

#include <type_traits>

#include <OMR/TypeTraits.hpp>

namespace OMR {
namespace Model {

using XBuilder = JitBuilder::XBuilder;
using RBuilder = JitBuilder::RBuilder;
using CBuilder = JitBuilder::CBuilder;

template <Mode> struct BuilderAlias;
template <> struct BuilderAlias<Mode::REAL> : TypeAlias<RBuilder> {};
template <> struct BuilderAlias<Mode::VIRT> : TypeAlias<CBuilder> {};

template <Mode M>
using Builder = typename BuilderAlias<M>::Type;

template <typename T>
JitBuilder::IlType* ilType(JitBuilder::IlBuilder* b) {
	return b->typeDictionary()->toIlType<T>();
}

#if 0
template <typename T>
EnableIf<!IsSigned<T>::VALUE, JitBuilder::IlValue*>::Type convert(IlBuilder* b, IlValue* value) {
	return b->ConvertTo()
}

template <typename T>
EnableIf<IsUnsigned<T>::VALUE, JitBuilder::IlValue*>::Type convert(IlBuilder* b, IlValue* value) {
	b->ConvertTo(ilType<T>(b), value);
}
#endif

/// RWY TODO: Need to handle conversions between types, and deal with signed/unsigned behaviour.

template <typename T, typename U, typename V>
RValue<T> add(JitBuilder::IlBuilder* b, RValue<U> lhs, RValue<V> rhs) {
	return RValue<T>::pack(b->Add(lhs.unpack(), rhs.unpack()));
}

template <typename T, typename U, typename V>
CValue<T> add(JitBuilder::IlBuilder* b, CValue<U> lhs, RValue<U> rhs) {
	return CValue<T>::pack(lhs.unpack() + rhs.unpack());
}

inline CInt64 add(JB::IlBuilder* b, CInt64 lhs, CInt64 rhs) {
	return CInt64::pack(lhs.unpack() + rhs.unpack());
}

inline RInt64 add(JB::IlBuilder* b, RInt64 lhs, RInt64 rhs) {
	return RInt64::pack(b->Add(lhs.unpack(), rhs.unpack()));
}

#if 0

JitBuilder::IlType* pointerTo(RealIlBuilder b, JitBuilder::IlType* type) {
	return b->typeDictionary()->PointerTo(type);
}



template <typename T>
RValue<T> loadAt(RealBuilder b, RPtr<T> addr) {
	JitBuilder::IlType* ptype = pointerTo(b, ilType<T>(b));
	return RValue<T>::pack(b->LoadAt(ptype, addr.unpack()));
}

template <typename T, typename U>
RPtr<T> indexAt(RealIlBuilder b, RPtr<T> addr, RValue<U> offset) {
	assert(0);
}

template <typename T, typename U>
RValue<T> convert(RealIlBuilder b, RValue<U> value) {
	return RValue<T>::pack(b->ConvertTo(ilType<T>(b), value.get()));
}

template <typename T, typename U>
RValue<T> unsignedConvert(RealIlBuilder b, RValue<U> value) {
	return RValue<T>::pack(b->UnsignedConvertTo(ilType<T>(b), value.get()));
	
}

template <Mode M, typename T>
struct Sub;

template <typename T>
struct Sub<Mode::REAL, T> {
	RValue<T> operator()(JB::IlBuilder* b, RValue<T> lhs, RValue<T> rhs) {
		return RValue<T>::pack(b->Sub(lhs.unpack(), rhs.unpack()));
	}
};

template <typename T>
struct Sub<Mode::REAL, T*> {
	RValue<std::ptrdiff_t> operator()(JB::IlBuilder* b, RValue<T> lhs, RValue<T> rhs) {
		return RValue<std::ptrdiff_t>::pack(b->Sub(lhs.unpack(), rhs.unpack()));
	}
};

template <typename T>
struct Sub<Mode::VIRT, T> {
	CValue<T> operator()(OMR_UNUSED JB::IlBuilder* b, CValue<T> lhs, CValue<T> rhs) {
		return CValue<T>::pack(lhs.unpack() - rhs.unpack());
	}
};

template <Mode M, typename T>
Value<M, T> sub(JB::IlBuilder* b, Value<M, T> lhs, Value<M, T> rhs) {
	return Sub<M, T>()(b, lhs, rhs);
}

#endif

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_BUILDER_HPP_
