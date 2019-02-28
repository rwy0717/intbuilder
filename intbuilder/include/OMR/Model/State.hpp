#if !defined(OMR_MODEL_STATE_HPP_)
#define OMR_MODEL_STATE_HPP_

#include "ilgen/IlBuilder.hpp"

namespace OMR {
namespace Model {

class State {};

class RealState : public State {
public:
	void initialize(TR::IlBuilder* b) {}

	void commit(TR::IlBuilder* b) {}

	void reload(TR::IlBuilder* b) {}

	void merge(TR::IlBuilder* b) {}
};

class VirtState : public State {};

class PureState : public State {};

}  // namespace Model
}  // namespace State

#endif // OMR_MODEL_STATE_HPP_
