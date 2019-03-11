#include <cstdio>
#include <TypeDictionary.hpp>
#include <MethodBuilder.hpp>
#include "Interpreter.hpp"
#include "JitHelpers.hpp"

namespace JB = OMR::JitBuilder;

/// Run a function in the interpreter.
void JitHelpers::interp_run(Interpreter* interpreter, Func* target) {
	interpreter->run(target);
}

/// Print a mini trace statement.
void JitHelpers::interp_trace(Interpreter* interpreter, Func* func) {
	fprintf(stderr, "    interpreter=%p func=%p: pc=%p=%hhu\n",
		interpreter, func, interpreter->_pc, *interpreter->_pc);
}

/// Print a debug string with line information
void JitHelpers::dbg_msg(const char* file, std::size_t line, const char* function, const char* msg) {
	fprintf(stderr, "%s:%zu: %s: %s\n", file, line, function, msg);
}

/// Print a string.
void JitHelpers::print_s(const char* str) {
	fprintf(stderr, "%s", str);
}

void JitHelpers::print_d(int i) {
	fprintf(stderr, "%d", i);
}

void JitHelpers::print_u(unsigned i) {
	fprintf(stderr, "%u", i);
}

void JitHelpers::print_x(unsigned i) {
	fprintf(stderr, "%x", i);
}

template <typename FPtr, typename... Args>
void defhelper(JB::MethodBuilder* b, const char* name, FPtr fptr, JB::IlType* rtype, Args... args) {
	b->DefineFunction(
		const_cast<char*>(name),
		"<jit-helper>", "<gen>",
		reinterpret_cast<void*>(fptr),
		rtype,
		sizeof...(args), args...
	);
}

void JitHelpers::define(JB::MethodBuilder* b) {
	JB::TypeDictionary* t = b->typeDictionary();

	defhelper(b, "interp_run", interp_run, t->NoType,
		t->PointerTo(t->LookupStruct("Interpreter")),
		t->PointerTo(t->LookupStruct("Func"))
	);

	defhelper(b, "interp_trace", interp_trace, t->NoType,
		t->PointerTo(t->LookupStruct("Interpreter")),
		t->PointerTo(t->LookupStruct("Func"))
	);

	defhelper(b, "dbg_msg", dbg_msg, t->NoType,
		t->PointerTo(t->Int8),
		t->Int64,
		t->PointerTo(t->Int8),
		t->PointerTo(t->Int8)
	);

	defhelper(b, "print_s", print_s, t->NoType,
		t->PointerTo(t->Int8)
	);

	defhelper(b, "print_d", print_d, t->NoType,
		t->Int32
	);

	defhelper(b, "print_u", print_x, t->NoType,
		t->Int32
	);

	defhelper(b, "print_x", print_x, t->NoType,
		t->Int32
	);
}
