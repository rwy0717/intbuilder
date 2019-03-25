#if !defined(OMR_MODEL_CONTROLFLOW_HPP_)
#define OMR_MODEL_CONTROLFLOW_HPP_

#include <OMR/Model/Mode.hpp>

#include <BytecodeBuilder.hpp>

#include <BytecodeBuilderTable.hpp>

namespace OMR {
namespace Model {

template <Mode M>
class ControlFlow;

template <>
class ControlFlow<Mode::REAL> {
public:
	void next(JB::IlBuilder* b, RSize offset) {
		_pcReg.store(b, RPtr<std::uint8_t>::pack(
			b->Add(
				_pcReg.unpack(),
				offset.unpack()
		)));
		b->Call("print_s", 1, b->Const((void*)"Pc updated: value="));
		b->Call("print_u", 1, _pcReg.unpack());
		b->Call("print_s", 1, b->Const((void*)"\n"));
	}

	void halt(JB::IlBuilder* b) {
		b->Return();
	}

	void halt(JB::IlBuilder* b, JB::IlValue* result) {
		b->Return(result);
	}

	void halt(JB::IlBuilder* b) {
		b->Return();
	}
};

template <>
class ControlFlow<Mode::VIRT> {
public:
	ControlFlow(JB::BytecodeBuilderTable* builders)  : _builders(builders) {}

	std::uintptr_t index(JB::BytecodeBuilder* b) const { return b->bcIndex(); }

	void next(JB::BytecodeBuilder* b, std::size_t index) {
		fprintf(stderr, "@@@ control-flow: NEXT bc-index:=%u target-bc-index=%zu\n", b->bcIndex(), index);
		b->AddFallThroughBuilder(_builders->get(b, index));
	}

	void IfCmpNotEqualZero(JB::BytecodeBuilder* b, JB::IlValue* cond, std::size_t index) {
		fprintf(stderr, "@@@ control-flow: IF_NOT_ZERO: bc-index:=%u target-bc-index=%zu\n", b->bcIndex(), index);
		b->IfCmpNotEqualZero(builders()->get(b, index), cond);
	}

	JB::BytecodeBuilderTable* builders() { return _builders; }

private:
	JB::BytecodeBuilderTable* _builders;
};

using VirtControlFlow = ControlFlow<Mode::VIRT>;
using RealControlFlow = ControlFlow<Mode::REAL>;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_CONTROLFLOW_HPP_
