#if !defined(OMR_MODEL_TYPES_HPP_)
#define OMR_MODEL_TYPES_HPP_

#include <OMR/Model.hpp>
#include <OMR/Model/Mode.hpp>
#include <OMR/TypeTraits.hpp>

#include <IlBuilder.hpp>
#include <IlValue.hpp>

#include <cstddef>
#include <cstdint>

namespace OMR {
namespace Model {

/// Helper for defining constants from custom types. Major Hack.
///
template <typename T>
struct Constant {
	JB::IlValue* operator()(JB::IlBuilder* b, T value) const {
		return b->Const(value);
	}
};

template <>
struct Constant<unsigned int> {
	JB::IlValue* operator()(JB::IlBuilder* b, unsigned long value) const {
		return b->Const(static_cast<std::int32_t>(value));
	}
};

template <>
struct Constant<long> {
	JB::IlValue* operator()(JB::IlBuilder* b, unsigned long value) const {
		return b->Const(static_cast<std::int64_t>(value));
	}
};

template <>
struct Constant<unsigned long> {
	JB::IlValue* operator()(JB::IlBuilder* b, long value) const {
		return b->Const(static_cast<std::int64_t>(value));
	}
};

template <typename T>
struct Constant<T*> {
	JB::IlValue* operator()(JB::IlBuilder* b, T* value) {
		return b->Const(static_cast<void*>(value));
	}
};

template <typename T>
struct Constant<const T*> {
	JB::IlValue* operator()(JB::IlBuilder* b, const T* value) const {
		return Constant<T*>()(b, const_cast<T*>(value));
	}
};

template <typename T>
JB::IlValue* constant(JB::IlBuilder* b, T value) {
	return Constant<T>()(b, value);
}

/// Modal value type. Values are either runtime or compile time.
///
template <Mode M, typename T>
struct Value;

template <typename T>
class Value<Mode::REAL, T> {
public:
	using Type = T;

	/// Construct from internal representation.
	///
	static Value<Mode::REAL, T> pack(JB::IlValue* value) { return Value<Mode::REAL, T>(value); }

	Value() = default;

	/// Construct as a const.
	///
	Value(JB::IlBuilder* b, T value) : _value(constant(b, value)) {}

	/// Convert to JitBuilder IL.
	/// For REAL values, this is a no-op.
	JB::IlValue* toIl(OMR_UNUSED JB::IlBuilder* b) { return _value; }

	/// Convert to internal representation.
	/// For REAL values, that is a JB::IlValue* representing a run-time value.
	JB::IlValue* unpack() const noexcept { return _value; }

	JB::IlValue* get() const noexcept { return _value; }

private:
	Value(JB::IlValue* value) : _value(value) {}

	JB::IlValue* _value;
};

template <typename T>
struct Value<Mode::VIRT, T> {
public:
	using Type = T;

	/// Construct from internal representation.
	///
	static Value<Mode::VIRT, T> pack(T value) { return Value<Mode::VIRT, T>(value); }

	Value() = default;

	/// Construct a compile-time constant value.
	///
	Value(OMR_UNUSED JB::IlBuilder* b, T value) : _value(value) {}

	/// Convert to jitbuilder IL.
	///
	JB::IlValue* toIl(JB::IlBuilder* b) const noexcept { return constant(b, _value); }

	/// Convert to internal representation.
	/// For VIRT values, that is a compile-time value of type T.
	T unpack() const noexcept { return _value; }

private:
	Value(T value) : _value(value) {}

	T _value;
};

/// @group Modal value types.
/// @{

template <Mode M, typename T> using Ptr = Value<M, T*>;
template <Mode M> using Int = Value<M, int>;
template <Mode M> using UInt = Value<M, unsigned int>;
template <Mode M> using UIntPtr = Value<M, std::uintptr_t>;
template <Mode M> using Size = Value<M, std::size_t>;
template <Mode M> using Int8 = Value<M, std::int8_t>;
template <Mode M> using Int16 = Value<M, std::int16_t>;
template <Mode M> using Int32 = Value<M, std::int32_t>;
template <Mode M> using Int64 = Value<M, std::int64_t>;
template <Mode M> using UInt8 = Value<M, std::uint8_t>;
template <Mode M> using UInt16 = Value<M, std::uint16_t>;
template <Mode M> using UInt32 = Value<M, std::uint32_t>;
template <Mode M> using UInt64 = Value<M, std::uint64_t>;
template <Mode M> using PtrDiff = Value<M, std::ptrdiff_t>;

/// @}
///

/// Runtime Value, aka "REAL" Value.
///
template <typename T>
using RValue = Value<Mode::REAL, T>;

/// @group Collection of runtime value types.
/// @{

template <typename T> using RPtr = RValue<T*>;
using RInt = RValue<int>;
using RUInt = RValue<unsigned int>;
using RUIntPtr = RValue<std::uintptr_t>;
using RSize = RValue<std::size_t>;
using RInt8 = RValue<std::int8_t>;
using RInt16 = RValue<std::int16_t>;
using RInt32 = RValue<std::int32_t>;
using RInt64 = RValue<std::int64_t>;
using RUInt8 = RValue<std::uint8_t>;
using RUInt16 = RValue<std::uint16_t>;
using RUInt32 = RValue<std::uint32_t>;
using RUInt64 = RValue<std::uint64_t>;
using RFloat = RValue<float>;
using RDouble = RValue<double>;
using RPtrDiff = RValue<std::ptrdiff_t>;

/// @}
///

/// Compile time value, aka "VIRT" value.
///
template <typename T>
using CValue = Value<Mode::VIRT, T>;

/// @group Collection of compile-time value types.
/// @{

template <typename T> using CPtr = CValue<T*>;
using CInt = CValue<int>;
using CUInt = CValue<unsigned int>;
using CIntPtr = CValue<std::intptr_t>;
using CUIntPtr = CValue<std::uintptr_t>;
using CSize = CValue<std::size_t>;
using CInt8 = CValue<std::int8_t>;
using CInt16 = CValue<std::int16_t>;
using CInt32 = CValue<std::int32_t>;
using CInt64 = CValue<std::int64_t>;
using CUInt8 = CValue<std::uint8_t>;
using CUInt16 = CValue<std::uint16_t>;
using CUInt32 = CValue<std::uint32_t>;
using CUInt64 = CValue<std::uint64_t>;
using CFloat = CValue<float>;
using CDouble = CValue<double>;
using CPtrDiff = CValue<std::ptrdiff_t>;

/// @}
///


}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_TYPES_HPP_
