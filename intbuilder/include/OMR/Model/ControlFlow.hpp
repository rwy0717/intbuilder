#if !defined(OMR_MODEL_CONTROLFLOW_HPP_)
#define OMR_MODEL_CONTROLFLOW_HPP_

#include <OMR/Model/Mode.hpp>
#include <OMR/Model/FunctionData.hpp>

#include <BytecodeBuilder.hpp>
#include <BytecodeBuilderTable.hpp>

namespace OMR {
namespace Model {

/// Interface to control flow operations.
/// All targets are ABSOLUTE, not relative.
template <Mode M>
class ControlFlow;

template <>
class ControlFlow<Mode::REAL> {
public:
	ControlFlow(FunctionData<Mode::REAL>& data) : _data(data), _address(nullptr) {}

	void initialize(JB::IlBuilder* b, JB::IlValue* address) {
		_address = address;
	}

	void next(RBuilder* b, JB::IlValue* index) {
		std::fprintf(stderr, "#####test\n");
		b->Call("print_s", 1, b->Const((void*)"$$$ ControlFlow next: index="));
		b->Call("print_u", 1, index);
		b->Call("print_s", 1, b->Const((void*)"\n"));

		JB::TypeDictionary* t = b->typeDictionary();
		JB::IlType* type = t->PointerTo(t->Int8);

		b->StoreAt(_address, b->Add(base(), index));
		b->GotoEnd();
		b->End()->Call("print_s", 1, b->End()->Const((void*) "$$$ AT END\n"));
		//b->End()->Return();
	}

	/// absolute control flow.
	void IfCmpNotEqualZero(RBuilder* b, JB::IlValue* cond, JB::IlValue* index) {

		b->Call("print_s", 1, b->Const((void*)"$$$ ControlFlow IfCmpNotEqualZero: cond= offset="));
		b->Call("print_u", 1, index);
		b->Call("print_s", 1, b->Const((void*)"\n"));

		JB::TypeDictionary* t = b->typeDictionary();
		JB::IlType* type = t->PointerTo(t->Int8);

		JB::IlBuilder* onTrue = nullptr;
		b->IfThen(&onTrue, cond);
		onTrue->Call("print_s", 1, onTrue->Const((void*)"$$$ ON TRUE TAKEN !!! \n"));
		onTrue->StoreAt(_address, onTrue->IndexAt(type, base(), index));
		onTrue->Goto(b->End());
	}

	void halt(JB::RBuilder* b) {
		b->End()->Return();
		b->Goto(b->End());
	}

	void halt(JB::IlBuilder* b, JB::IlValue* result) {
		b->Return(result);
	}

private:
	JB::IlValue* base() const { return _data.start(); }

	const FunctionData<Mode::REAL>& _data;
	JB::IlValue* _address;
};

template <>
class ControlFlow<Mode::VIRT> {
public:
	ControlFlow(FunctionData<Mode::VIRT>& data) : _address(nullptr), _data(data) {}

	std::uintptr_t index(JB::BytecodeBuilder* b) const { return b->bcIndex(); }

	void initialize(JB::IlBuilder* b, JB::IlValue* address)  {
		_address = address;
	}

	void next(JB::BytecodeBuilder* b, std::size_t index) {
		fprintf(stderr, "@@@ control-flow: NEXT bc-index:=%u target-bc-index=%zu\n", b->bcIndex(), index);
		b->AddFallThroughBuilder(builders()->get(b, index));
	}

	/// absolute control flow.
	void IfCmpNotEqualZero(JB::BytecodeBuilder* b, JB::IlValue* cond, std::size_t index) {
		fprintf(stderr, "@@@ control-flow: IF_NOT_ZERO: bc-index:=%u target-bc-index=%zu\n", b->bcIndex(), index);
		b->IfCmpNotEqualZero(builders()->get(b, index), cond);
	}

	void halt(JB::IlBuilder* b) {
		b->Return();
	}

	void halt(JB::IlBuilder* b, JB::IlValue* result) {
		b->Return(result);
	}

private:
	JB::BytecodeBuilderTable* builders() const { return data().builders(); }

	const FunctionData<Mode::VIRT>& data() const { return _data; }

	const FunctionData<Mode::VIRT>& _data;
	JB::IlValue* _address;
};

using VirtControlFlow = ControlFlow<Mode::VIRT>;
using RealControlFlow = ControlFlow<Mode::REAL>;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_CONTROLFLOW_HPP_
