#if !defined(OMR_MODEL_HPP_)
#define OMR_MODEL_HPP_

#include <cstddef>
#include <cstdint>

#include <OMR/TypeTraits.hpp>

#include <MethodBuilder.hpp>

namespace OMR {

namespace Model {}

namespace JitBuilder {}
namespace JB = JitBuilder;

template <std::size_t> struct FixInt;
template <> struct FixInt<1> : TypeAlias<std::int8_t> {};
template <> struct FixInt<2> : TypeAlias<std::int16_t> {};
template <> struct FixInt<4> : TypeAlias<std::int32_t> {};
template <> struct FixInt<8> : TypeAlias<std::int64_t> {};

template <typename T> struct ToFixInt : FixInt<sizeof(T)> {};

template <typename T>
auto fixint_cast(T x) -> typename ToFixInt<T>::Type {
	return static_cast<typename ToFixInt<T>::Type>(x);
}

template <typename T> struct RemoveUnsigned : TypeAlias<T> {};
template <> struct RemoveUnsigned<std::uint8_t> : TypeAlias<std::int8_t> {};
template <> struct RemoveUnsigned<std::uint16_t> : TypeAlias<std::int16_t> {};
template <> struct RemoveUnsigned<std::uint32_t> : TypeAlias<std::int32_t> {};
template <> struct RemoveUnsigned<std::uint64_t> : TypeAlias<std::int64_t> {};
template <> struct RemoveUnsigned<unsigned long> : TypeAlias<std::int64_t> {};

} // namespace OMR

#define OMR_UNUSED __attribute__((unused))

#define OMR_STRINGIFY_NOEXPAND(x) #x
#define OMR_STRINGIFY(x) OMR_STRINGIFY_NOEXPAND(x)
#define OMR_LINE_STR OMR_STRINGIFY(__LINE__)

#endif // OMR_MODEL_HPP_
