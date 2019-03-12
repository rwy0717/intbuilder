#if !defined(OMR_MODEL_BUILDER_HPP_)
#define OMR_MODEL_BUILDER_HPP_

#include <OMR/Model.hpp>
#include <OMR/Model/Mode.hpp>

#include <IlBuilder.hpp>
#include <MethodBuilder.hpp>
#include <BytecodeBuilder.hpp>

namespace OMR {
namespace Model {

template <Mode M> class Builder;

template <> class Builder<Mode::VIRT> : public JB::BytecodeBuilder {};

template <> class Builder<Mode::REAL> : public JB::IlBuilder {};

using VirtBuilder = Builder<Mode::VIRT>;
using RealBuilder = Builder<Mode::REAL>;

}  // namespace Model
}  // namespace OMR

#endif // OMR_MODEL_BUILDER_HPP_
