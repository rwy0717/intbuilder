#if !defined(OMR_JITBUILDER_BYTECODEINTERPRETERBUILDER_HPP_)
#define OMR_JITBUILDER_BYTECODEINTERPRETERBUILDER_HPP_

#include <MethodBuilder.hpp>
#include <VirtualMachineState.hpp>
#include <TypeDictionary.hpp>

#include <cstdint>
#include <cstddef>
#include <map>
#include <vector>

namespace OMR {
namespace JitBuilder {

class BytecodeInterpreterBuilder : public MethodBuilder {
public:
	class Handler {
	public:
		virtual ~Handler() = default;

		virtual bool invoke(IlBuilder* b, VirtualMachineState* state) = 0;
	};

	template <typename HandlerT, typename VmStateT>
	class Wrapper final : public Handler {
	public:
		explicit Wrapper(const HandlerT& handler) : _handler(handler) {}

		virtual ~Wrapper() override final {}

		virtual bool invoke(IlBuilder* b, VirtualMachineState* state) override final {
			assert(state != nullptr);
			return _handler(b, *static_cast<VmStateT*>(state));
		}

	private:
		HandlerT _handler;
	};

	class HandlerTableBase {
	public:
		using HandlerMap = std::map<std::uint32_t, std::unique_ptr<Handler>>;
		
		HandlerMap* data() { return &_handlers; }

		HandlerMap::iterator begin() { return _handlers.begin(); }

		HandlerMap::iterator end() { return _handlers.end(); }

		Handler* getDefault() { return _default.get(); }

		Handler* get(std::uint32_t opcode) { return _handlers[opcode].get(); }

	protected:
		HandlerTableBase() = default;

		HandlerMap _handlers;
		std::unique_ptr<Handler> _default;

	private:
		friend class BytecodeInterpreterBuilder;
	};

	template <typename VmStateT>
	class HandlerTable final : public HandlerTableBase {
	public:
		HandlerTable() = default;

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
		std::unique_ptr<Wrapper<HandlerT, VmStateT>>
		wrap(const HandlerT& handler) {
			return std::unique_ptr<Wrapper<HandlerT, VmStateT>>(new Wrapper<HandlerT, VmStateT>(handler));
		}
	};

	BytecodeInterpreterBuilder(TypeDictionary* t, HandlerTableBase* handlers)
		: MethodBuilder(t)
		, _handlers(handlers) {
		DefineLocal("interpreter_opcode",   t->Int32);
		DefineLocal("interpreter_continue", t->Int32);
	}

	~BytecodeInterpreterBuilder() = default;

	virtual IlValue* getOpcode(IlBuilder* b) = 0;

	bool buildInterpreterIL(VirtualMachineState* state) {

		assert(state != nullptr);

		Store("interpreter_opcode",   Const(std::int32_t(-1)));
		Store("interpreter_continue", Const(std::int32_t(1)));

		IlBuilder* loop = OrphanBuilder();
		DoWhileLoop((char*)"interpreter_continue", &loop);

		// state->Reload(loop);
		IlValue* opcode = getOpcode(loop);
		loop->Store("interpreter_opcode", opcode);
	
		loop->Call("print_s", 1, loop->Const((void*)"$$$ *** INTERPRETING: opcode="));
		loop->Call("print_u", 1, loop->Load("interpreter_opcode"));
		loop->Call("print_s", 1, loop->Const((void*)"\n"));

		IlBuilder* defaultHandler = genDefaultHandler(state);
		std::vector<IlBuilder::JBCase*> handlers = genHandlers(state);

		loop->Switch("interpreter_opcode", &defaultHandler, handlers.size(), handlers.data());

		return true;
	}

private:
	IlBuilder* genDefaultHandler(VirtualMachineState* state) {

		if (_handlers->getDefault() == nullptr) {
			return nullptr;
		}

		IlBuilder* b = OrphanBuilder();
		VirtualMachineState* copy = state->MakeCopy();
		copy->Reload(b);
		_handlers->getDefault()->invoke(b, copy);
		delete copy;
		return b;
	}

	std::vector<IlBuilder::JBCase*> genHandlers(VirtualMachineState* state) {
		std::vector<IlBuilder::JBCase*> cases;
		for(const auto& node : *_handlers) {
			IlBuilder* b = OrphanBuilder();
			cases.push_back(MakeCase(node.first, &b, false));
			VirtualMachineState* copy = state->MakeCopy();
			copy->Reload(b);
			node.second->invoke(b, copy);
			delete copy;
		}
		return cases;
	}

private:
	HandlerTableBase* _handlers;
};

}  // namespace JitBuilder
}  // namespace OMR

#endif // OMR_JITBUILDER_BYTECODEINTERPRETERBUILDER_HPP_
