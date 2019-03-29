#if !defined(OMR_JITBUILDER_BYTECODEHANDLERTABLE_HPP_)
#define OMR_JITBUILDER_BYTECODEHANDLERTABLE_HPP_

#include <BytecodeBuilder.hpp>
#include <BytecodeBuilderTable.hpp>
#include <MethodBuilder.hpp>

#include <functional>
#include <map>
#include <memory>

namespace OMR {
namespace JitBuilder {

class BytecodeHandlerEntry {
public:
	virtual ~BytecodeHandlerEntry() = default;

	virtual bool invoke(CBuilder* b) = 0;
};

template <typename HandlerT, typename VmStateT>
class BytecodeHandlerWrapper final : public BytecodeHandlerEntry {
public:
	explicit BytecodeHandlerWrapper(const HandlerT& handler) : _handler(handler) {}

	virtual ~BytecodeHandlerWrapper() override final {}

	virtual bool invoke(CBuilder* b) override final {
		VmStateT* state = static_cast<VmStateT*>(b->vmState());
		assert(state != nullptr);
		fprintf(stderr, "@@@ invoking bc-builder=%p vm-state=%p\n", b, state);
		return _handler(b, *state);
	}

private:
	HandlerT _handler;
};

class BytecodeHandlerTableBase {
public:
	~BytecodeHandlerTableBase() = default;

	bool invoke(CBuilder* b, std::uint32_t opcode) {
		bool success = false;
		if (_handlers.count(opcode) != 0) {
			success = _handlers[opcode]->invoke(b);
		} else if (_default != nullptr) {
			success = _default->invoke(b);
		} else {
			success = false;
		}
		return success;
	}

protected:
	BytecodeHandlerTableBase() = default;

	std::map<std::uint32_t, std::unique_ptr<BytecodeHandlerEntry>> _handlers;
	std::unique_ptr<BytecodeHandlerEntry> _default;
};

template <typename VmStateT>
class BytecodeHandlerTable final : public BytecodeHandlerTableBase {
public:
	BytecodeHandlerTable() = default;

	template <typename HandlerT>
	void set(std::uint32_t opcode, const HandlerT& handler) {
		fprintf(stderr, "create handler bc=%x handler=%p\n", opcode, &handler);
		assert(_handlers.count(opcode) == 0); // do not allow double inserts.
		_handlers[opcode] = wrap(handler);
	}

	template <typename HandlerT>
	void setDefault(const HandlerT& handler) {
		fprintf(stderr, "create default handler handler=%p\n", &handler);
		assert(_default == nullptr); // do not allow to reset the default handler.
		_default = wrap(handler);
	}

private:
	template <typename HandlerT>
	std::unique_ptr<BytecodeHandlerWrapper<HandlerT, VmStateT>>
	wrap(const HandlerT& handler) {
		return std::unique_ptr<BytecodeHandlerWrapper<HandlerT, VmStateT>>(new BytecodeHandlerWrapper<HandlerT, VmStateT>(handler));
	}
};

}  // namespace JitBuilder
}  // namespace OMR

#endif // OMR_JITBUILDER_BYTECODEHANDLERTABLE_HPP_
