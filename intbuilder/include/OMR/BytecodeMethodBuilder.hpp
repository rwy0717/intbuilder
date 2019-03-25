#if !defined(OMR_JITBUILDER_BYTECODEMETHODBUILDER_HPP_)
#define OMR_JITBUILDER_BYTECODEMETHODBUILDER_HPP_

#include <BytecodeHandlerTable.hpp>
#include <MethodBuilder.hpp>
#include <BytecodeBuilder.hpp>

#include <functional>
#include <map>
#include <memory>

namespace OMR {
namespace JitBuilder {

/// Bytecode-driven method builder.
class BytecodeMethodBuilder : public MethodBuilder {
public:
	BytecodeMethodBuilder(TypeDictionary* typeDictionary, BytecodeHandlerTableBase* handlers)
		: MethodBuilder(typeDictionary)
		, _handlers(handlers) {}

	bool buildBytecodeIL() {
		AppendBuilder(_builders.get(this, 0));
		std::int32_t index = -1;
		while((index = GetNextBytecodeFromWorklist()) != -1) {
			std::uint32_t opcode = getOpcode(index);
			BytecodeBuilder* builder = _builders.get(this, index);
			assert(index == builder->bcIndex());

			fprintf(stderr, "@@@ *** compiling index=%u opcode=%u\n", index, opcode);
			fprintf(stderr, "@@@ *** compiling bc-builder=%p vm-state=%p\n", builder, builder->vmState());
	
			builder->Call("print_s", 1, builder->Const((void*)"$$$ *** START index="));
			builder->Call("print_u", 1, builder->Const((std::int32_t)index));
			builder->Call("print_s", 1, builder->Const((void*)" opcode="));
			builder->Call("print_u", 1, builder->Const((std::int32_t)opcode));
			builder->Call("print_s", 1, builder->Const((void*)"\n"));

			bool success = _handlers->invoke(builder, opcode);
			if (!success) {
				return false;
			}
		}
		return true;
	}

	virtual std::uint32_t getOpcode(std::size_t index) = 0;

	BytecodeBuilderTable* builders() { return &_builders; }

private:
	BytecodeHandlerTableBase* _handlers;
	BytecodeBuilderTable _builders;
};

}  // namespace JitBuilder
}  // namespace OMR

#endif // OMR_JITBUILDER_BYTECODEMETHODBUILDER_HPP_
