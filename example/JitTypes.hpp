#if !defined(JITTYPES_HPP_)
#define JITTYPES_HPP_

#include <TypeDictionary.hpp>

class JitTypes {
public:
	static void define(OMR::JitBuilder::TypeDictionary* t);

private:
	static void defineFunc(OMR::JitBuilder::TypeDictionary* t);

	static void defineInterpreter(OMR::JitBuilder::TypeDictionary* t);
};

#endif // JITTYPES_HPP_
