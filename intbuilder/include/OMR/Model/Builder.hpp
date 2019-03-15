#if !defined(OMR_MODEL_BUILDER_HPP_)
#define OMR_MODEL_BUILDER_HPP_

#include <OMR/Model.hpp>
#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Value.hpp>

#include <IlBuilder.hpp>
#include <MethodBuilder.hpp>
#include <BytecodeBuilder.hpp>

#include <type_traits>

#include <OMR/TypeTraits.hpp>

namespace OMR {
namespace Model {

template <Mode M>
struct IlBuilderAlias;

template <>
struct IlBuilderAlias<Mode::REAL> : TypeAlias<JitBuilder::BytecodeBuilder> {};

template <>
struct IlBuilderAlias<Mode::VIRT> : TypeAlias<JitBuilder::IlBuilder> {};

template <Mode M>
using IlBuilder = typename IlBuilderAlias<M>::Type;

#if 0 ////////////////////////////////////////

template <>
class Builder<Mode::REAL> : public JitBuilder::IlBuilder {
public:
	explicit Builder(JitBuilder::IlBuilder* b) : _b(b) {}

	JitBuilder::IlBuilder* operator->() const noexcept { return _b; }

	JitBuilder::IlBuilder& operator*() const noexcept { return *_b; }

	JitBuilder::TypeDictionary* typedict() { return _b->typeDictionary(); }

	JitBuilder::IlBuilder* ilbuilder() const { return _b; }

private:

	template <typename T>
	JitBuilder::IlType* ToIlType() { return typedict()->toIlType<T>(); }

	JitBuilder::IlBuilder* _b;
};

template <>
class Builder<Mode::VIRT> : public JitBuilder::BytecodeBuilder {
public:
	static Builder<Mode::VIRT> pack(JitBuilder::IlBuilder* b) {
		return Builder(b);
	}

	JitBuilder::IlBuilder* unpack() const { return _b; }

private:
	explicit Builder(JitBuilder::IlBuilder* b) : _b(b) {}

	JitBuilder::IlBuilder* _b;
};



using RealIlBuilder = IlBuilder<Mode::REAL>;
using VirtIlBuilder = IlBuilder<Mode::VIRT>;

template <typename T>
JitBuilder::IlType* ilType(RBuilder b) {
	return b->typeDictionary()->toIlType<T>();
}

JitBuilder::IlType* pointerTo(RBuilder b, JitBuilder::IlType* type) {
	return b->typeDictionary()->PointerTo(type);
}

template <typename T, typename U>
RValue<T> add(RBuilder b, RValue<T> lhs, RValue<U> rhs) {
	return RValue<T>(b->Add(lhs.get(), rhs.get()));
}

template <typename T>
RValue<T> loadAt(RBuilder b, RPtr<T> addr) {
	JitBuilder::IlType* ptype = pointerTo(b, ilType<T>(b));
	return RValue<T>::pack(b->LoadAt(ptype, addr.unpack()));
}

template <typename T, typename U>
RPtr<T> indexAt(RBuilder b, RPtr<T> addr, RValue<U> offset) {
	assert(0);
}

template <typename T, typename U>
RValue<T> convert(RBuilder b, RValue<U> value) {
	return RValue<T>::pack(b->ConvertTo(ilType<T>(b), value.get()));
}

template <typename T, typename U>
RValue<T> unsignedConvert(RBuilder b, RValue<U> value) {
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

#endif ////////////////////////////////////////

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_BUILDER_HPP_
