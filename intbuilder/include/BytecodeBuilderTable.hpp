#if !defined(OMR_JITBUILDER_BYTECODEBUILDERTABLE_HPP_)
#define OMR_JITBUILDER_BYTECODEBUILDERTABLE_HPP_

#include <MethodBuilder.hpp>
#include <BytecodeBuilder.hpp>

#include <map>

namespace OMR {
namespace JitBuilder {

class BytecodeBuilderTable {
public:
	using Map = std::map<std::size_t, BytecodeBuilder*>;
	using Iter = Map::iterator;
	using Node = std::pair<const std::size_t, BytecodeBuilder*>;

	BytecodeBuilder* get(MethodBuilder* mb, std::size_t index) {
		std::pair<Iter, bool> search = _map.insert(Node(index, nullptr));
		Iter iter = search.first;
		bool found = search.second;
		if (found) {
			assert(iter->second == nullptr);
			iter->second = mb->OrphanBytecodeBuilder(index, strdup("unknown-bytecode"));
		}
		assert(iter->second != nullptr);
		return iter->second;
	}

private:
	Map _map;
};

}  // namespace JitBuilder
}  // namespace OMR

#endif // OMR_JITBUILDER_BYTECODEBUILDERTABLE_HPP_
