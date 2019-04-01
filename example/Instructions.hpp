#if !defined(INSTRUCTIONS_HPP_)
#define INSTRUCTIONS_HPP_

#include <cstddef>
#include <cstdint>

enum class Op : std::uint8_t {
	UNKNOWN,
	HALT,
	NOP,
	PUSH_CONST,
	ADD,
	PUSH_LOCAL,
	POP_LOCAL,
	BRANCH_IF,
	CALL,
	COUNT_
};

constexpr std::size_t OPCOUNT = std::size_t(Op::COUNT_);

#endif // INSTRUCTIONS_HPP_
