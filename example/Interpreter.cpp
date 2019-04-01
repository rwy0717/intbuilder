#include <Interpreter.hpp>
#include <BytecodeMethodBuilder.hpp>
#include <BytecodeInterpreterBuilder.hpp>

InterpretFn Interpreter::_interpret = nullptr;

InterpretFn Interpreter::compile_interpret_fn() {
	BytecodeInterpreterCompiler compiler;
	BytecodeInterpreterBuilder builder(&compiler);
	void* interpret = nullptr;
	std::int32_t rc = compileMethodBuilder(&builder, &interpret);
	if (rc != 0) {
		fprintf(stderr, "Failed to compile interpret fn\n");
		assert(0);
	}
	return (InterpretFn)interpret;
}

void Interpreter::compile(Func* func) {
	assert(func->cbody == nullptr);
	BytecodeMethodBuilder builder(&_compiler, func);
	std::int32_t rc = compileMethodBuilder(&builder, (void**)&func->cbody);
	if (rc != 0) {
		fprintf(stderr, "Failed to compile %p\n", func);
		assert(0);
	}
}
