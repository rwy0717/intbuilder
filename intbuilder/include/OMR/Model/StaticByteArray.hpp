#if !defined(OMR_MODEL_STATICBYTEARRAY_HPP_)
#define OMR_MODEL_STATICBYTEARRAY_HPP_

#include <OMR/Model/Mode.hpp>
#include <OMR/Model/Value.hpp>
#include <OMR/Model/Builder.hpp>

namespace OMR {
namespace Model {

template <Mode M>
class StaticByteArray {
public:
	StaticByteArray() = default;

	StaticByteArray(Ptr<M, std::uint8_t> addr, Size<M> length)
		: _addr(addr), _length(length) {}

	void initialize(Ptr<M, std::uint8_t> addr, Size<M> length) {
		_addr = addr;
		_length = length;
	}

	/// Load at offset.
	template <typename T>
	Value<M, T> load(Builder<M> b, Size<M> offset) {
		load(b, add(b, _addr, offset));
	}

	/// Load at constant offset.
	template <typename T>
	Value<M, T> load(Builder<M> b, std::size_t offset) {
		return load(b, Size<M>(b, offset));
	}

	/// Load at a dynamically calculated offset.
	JitBuilder::IlValue* load(Builder<M> b, JitBuilder::IlValue* offset) {
		return b->LoadAt(b->Add(_addr.toIl(), b->Const(offset)));
	}

	Ptr<M, std::uint8_t> addr() const { return _addr; }

	Size<M> length() const { return _length; }

private:
	Ptr<M, std::uint8_t> _addr;
	Size<M> _length;
};

using RealStaticByteArray = StaticByteArray<Mode::REAL>;
using VirtStaticByteArray = StaticByteArray<Mode::VIRT>;

} // namespace Model
} // namespace OMR

#endif // OMR_MODEL_STATICBYTEARRAY_HPP_
