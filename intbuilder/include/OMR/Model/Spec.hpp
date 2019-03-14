#if !defined(OMR_MODEL_SPEC_HPP_)
#define OMR_MODEL_SPEC_HPP_

namespace OMR {
namespace Model {

template <typename MachineT>
class Spec {
public:
	using MachineType = MachineT;
	using GeneratorType = void(JB::IlBuilder* b, MachineT&);
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_SPEC_HPP_
