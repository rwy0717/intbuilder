#if !defined(JITHELPERS_HPP_)
#define JITHELPERS_HPP_

#include <MethodBuilder.hpp>

#include <cstdint>
#include <cstddef>
#include <cstdlib>

namespace JB = OMR::JitBuilder;

class Interpreter;
struct Func;

class JitHelpers {
public:
	static void define(JB::MethodBuilder* b);

private:
	/// Run a function in the interpreter.
	static void interp_run(Interpreter* interpreter, Func* target);

	/// Print a mini trace statement.
	static void interp_trace(Interpreter* interpreter, Func* func);

	/// Print a debug string with line information
	static void dbg_msg(const char* file, std::size_t line, const char* function, const char* msg);

	/// Print "%s"
	static void print_s(const char* s);

	/// print "%d"
	static void print_d(std::intptr_t d);

	/// print "%x"
	static void print_x(std::uintptr_t x);

	/// print "%u"
	static void print_u(std::uintptr_t u);
};

#endif // JITHELPERS_HPP_
