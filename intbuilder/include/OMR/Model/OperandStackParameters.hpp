#if !defined(OMR_MODEL_OPERANDSTACKPARAMETERS_HPP_)
#define OMR_MODEL_OPERANDSTACKPARAMETERS_HPP_

// TODO RWY

#include <OMR/Model/Mode.hpp>

#include <IlBuilder.hpp>

namespace OMR {
namespace Model {

template <Mode M>
class OperandStackParameters;

template <>
class OperandStackParameters<Mode::REAL> {
public:
	void initialize()

	JitBuilder::IlValue* get(RBuilder* b, RSize index) {
		return loadAt(b, indexAt(b, _addr, index));
	}

private:
	JitBuilder::IlValue* l
	JitBuilder::IlValue* _addr;
};

template <>
class OperandStackParameters<Mode::VIRT> {
public:
	void initialize()

	JitBuilder::IlValue* get(CSize n) {
		return Load("Param0");
	}

private:
	std::size_t _nparams;
	JitBuilder::IlValue* _addr;
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_OPERANDSTACKPARAMETERS_HPP_
