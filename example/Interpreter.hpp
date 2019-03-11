#if !defined(INTERPRETER_HPP_)
#define INTERPRETER_HPP_

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cassert>

class Interpreter;
class JitTypes;
class JitHelpers;
struct Func;

enum class Op : std::uint8_t {
	UNKNOWN, HALT, NOP, PUSH_CONST, ADD, PUSH_LOCAL, POP_LOCAL, CALL,
	LAST = CALL
};

constexpr std::size_t OPCOUNT = std::size_t(Op::LAST);

/// The main interpreter function type. Generated by JitBuilder.
using InterpretFn = void(*)(Interpreter*, Func*);

/// Any JIT-compiled function.
using CompiledFn = void(*)(Interpreter*);

/// Function header.
struct Func {
	CompiledFn cbody = nullptr; //< compiled body ptr.
	std::size_t nlocals = 0;
	std::size_t nparams = 0;
	std::uint8_t body[]; //< bytecode body. trailing data.
};

/// The interpreter state.
class Interpreter {
public:
	static constexpr std::size_t  STACK_SIZE = 4*8; //< in bytes
	static constexpr std::uint8_t POISON     = 0x5e;

	Interpreter() :
		_sp(nullptr), _pc(nullptr), _fp(nullptr), _interpret(nullptr) {
		std::memset(_stack, POISON, STACK_SIZE);
		initialize();
	}

	void run(Func* target) {
		if (target->cbody != nullptr) {
			target->cbody(this);
		} else {
			interpret(target);
		}
	}

	void interpret(Func* target) {
		assert(_interpret != nullptr);
		_interpret(this, target);
	}

	std::int64_t peek(std::size_t offset = 0) const {
		return reinterpret_cast<const std::int64_t*>(_stack)[offset];
	}

	const std::uint8_t* sp() const { return _sp; }

private:
	friend class JitHelpers;
	friend class JitTypes;

	void initialize() {
		_sp = _stack;
	}

	std::uint8_t* _sp;                //< Stack pointer. Pointer to top of stack.
	std::uint8_t* _pc;                //< Program counter. Pointer to current bytecode.
	std::uint8_t* _startpc;           //< pc at function entry. Used for absolute jumps.
	Func* _fp;                        //< Function pointer. Pointer to current function.
	InterpretFn _interpret;           //< Interpreter main function. Generated by JitBuilder.
	std::uint8_t _stack[STACK_SIZE];
};

#endif // INTERPRETER_HPP_
