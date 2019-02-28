#if !defined(OMR_MODEL_MODE_HPP_)
#define OMR_MODEL_MODE_HPP_

namespace OMR {
namespace Model {

enum class Mode {
	REAL, // Interpreter mode. The model is a real object.
	VIRT, // Compiler mode. The model is virtual, tracking reads and writes for compilation.
	PURE, // The model is pure-virtual. There are no side effects to commit.
};

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_MODE_HPP_
