#if !defined(OMR_MODEL_SPEC_HPP_)
#define OMR_MODEL_SPEC_HPP_

namespace OMR {
namespace Model {

template <typename MachineT>
class Spec {
public:
	using MachineType = MachineT;
	using GeneratorType = void(JB::IlBuilder* b, MachineT&);

	void initialize(JB::MethodBuilder* b) {}

	void tearDown(JB::MethodBuilder* b) {}

	JB::IlValue* dispatch(JB::IlBuilder* b) {}

	void genUnhandled(JB::IlBuilder* b, MachineT machine) {
		DefaultHandler<Model::Mode::REAL>().build(clone, defaultHandler);
	}

private:
	std::vector<Generator> generators_;
	Generator unhandled_;
	ValueGenerator dispatch_;
};
zd
}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_SPEC_HPP_
