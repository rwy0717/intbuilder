
#include "JitTypes.hpp"
#include "Interpreter.hpp"
#include <TypeDictionary.hpp>

namespace JB = OMR::JitBuilder;

void JitTypes::define(JB::TypeDictionary* t) {
	JitTypes::defineFunc(t);
	JitTypes::defineInterpreter(t);
}

void JitTypes::defineFunc(JB::TypeDictionary* t) {
	t->DefineStruct("Func");
	t->DefineField("Func", "cbody",   t->Address, offsetof(Func, cbody));
	t->DefineField("Func", "nlocals", t->Word,    offsetof(Func, nlocals));
	t->DefineField("Func", "nparams", t->Word,    offsetof(Func, nparams));
	t->DefineField("Func", "body",    t->NoType,  offsetof(Func, body));
	t->CloseStruct("Func");
}

void JitTypes::defineInterpreter(JB::TypeDictionary* t) {
	t->DefineStruct("Interpreter");
	t->DefineField("Interpreter", "_sp",        t->pInt64,                             offsetof(Interpreter, _sp));
	t->DefineField("Interpreter", "_pc",        t->pInt8,                              offsetof(Interpreter, _pc));
	t->DefineField("Interpreter", "_startpc",   t->pInt8,                              offsetof(Interpreter, _startpc));
	t->DefineField("Interpreter", "_fp",        t->PointerTo(t->LookupStruct("Func")), offsetof(Interpreter, _fp));
	t->DefineField("Interpreter", "_interpret", t->Address,                            offsetof(Interpreter, _interpret));
	t->DefineField("Interpreter", "_stack",     t->NoType,                             offsetof(Interpreter, _stack));
	t->CloseStruct("Interpreter");
}
