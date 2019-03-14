#if !defined(OMR_STRSYMMANAGER_HPP_)
#define OMR_STRSYMMANAGER_HPP_

#include <map>

namespace OMR {

struct SymbolId {
	std::uintptr_t value;
};

class StrSymManager {
public:
	/// Take ownership of str. str will be freed when the manager is destroyed.
	void own(const char* str) {
	}

	/// intern a string for use by.
	char* intern(const char*)

	char* intern(const std::string& )

	template <typename... Args>
	char* intern(Args&&... args) {
		return intern(concat_str(std::forward<Args>(args)...));
	}

	str(SymbolId) 

private:
	std::uintptr_t counter;
	std::map<SymbolId, std::string> _symbols;
	std::map<std::string, SymbolId> _ids;
};

}

#endif // OMR_STRSYMMANAGER_HPP_
