#if !defined(BYTECODEMETHODBUILDER_HPP_)
#define BYTECODEMETHODBUILDER_HPP_

#include <OMR/BytecodeMethodBuilder.hpp>
#include <OMR/Model/Mode.hpp>

#include <Instructions.hpp>

class Func;

namespace Model {
template <OMR::Model::Mode> class Machine;
}  // namespace Model

class BytecodeMethodCompiler {
public:
	static constexpr OMR::Model::Mode M = OMR::Model::Mode::VIRT;

	using BytecodeHandlerTable =
		OMR::JitBuilder::BytecodeHandlerTable<Model::Machine<M>>;

	BytecodeMethodCompiler();

	OMR::JitBuilder::TypeDictionary* typedict() { return &_typedict; }

	OMR::JitBuilder::BytecodeHandlerTable<Model::Machine<M>>* handlers() { return &_handlers; }

private:
	template <typename HandlerT>
	void set(Op op, const HandlerT& handler) {
		_handlers.set(std::uint32_t(op), handler);
	}

	OMR::JitBuilder::TypeDictionary _typedict;
	OMR::JitBuilder::BytecodeHandlerTable<Model::Machine<M>> _handlers;
};

class BytecodeMethodBuilder : public OMR::JitBuilder::BytecodeMethodBuilder {
public:
	BytecodeMethodBuilder(BytecodeMethodCompiler* compiler, Func* func);

	virtual std::uint32_t getOpcode(std::size_t index) override final;

	virtual bool buildIL() override final;

private:
	Func* _func;
};

#endif // BYTECODEMETHODBUILDER_HPP_
