#if !defined(BYTECODEINTERPRETERBUILDER_HPP_)
#define BYTECODEINTERPRETERBUILDER_HPP_

#include <BytecodeHandlers.hpp>
#include <Instructions.hpp>
#include <OMR/Model/Mode.hpp>
#include <memory>
#include <cstdint>

namespace Model {
template <OMR::Model::Mode> class Machine;
} // namespace Model

class BytecodeInterpreterCompiler {
public:
	static constexpr Model::Mode M = Model::Mode::REAL;

	using HandlerTable = 
		OMR::JitBuilder::BytecodeInterpreterBuilder::HandlerTable<Model::Machine<M>>;

	BytecodeInterpreterCompiler();

	HandlerTable* handlers() { return &_handlers; }

	const HandlerTable* handlers() const { return &_handlers; }

	OMR::JitBuilder::TypeDictionary* typedict() { return &_typedict; }

	const OMR::JitBuilder::TypeDictionary* typedict() const { return &_typedict; }

private:
	template <typename HandlerT>
	void set(Op op, const HandlerT& handler) {
		_handlers.set(std::uint32_t(op), handler);
	}

	OMR::JitBuilder::TypeDictionary _typedict;
	HandlerTable _handlers;
};

class BytecodeInterpreterBuilder final : public OMR::JitBuilder::BytecodeInterpreterBuilder {
public:
	static constexpr Model::Mode M = Model::Mode::REAL;

	BytecodeInterpreterBuilder(BytecodeInterpreterCompiler* compiler);

	virtual OMR::JitBuilder::IlValue* getOpcode(OMR::JitBuilder::IlBuilder* b) override;

	virtual bool buildIL() override final;

private:
	std::unique_ptr<Model::Machine<Model::Mode::REAL>> _machine;
};

#endif // BYTECODEINTERPRETERBUILDER_HPP_
