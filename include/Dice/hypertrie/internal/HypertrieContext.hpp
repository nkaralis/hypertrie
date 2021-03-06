#ifndef HYPERTRIE_HYPERTRIECONTEXT_HPP
#define HYPERTRIE_HYPERTRIECONTEXT_HPP


#include "Dice/hypertrie/internal/ConfigHypertrieDepthLimit.hpp"
#include "Dice/hypertrie/internal/raw/storage/NodeContext.hpp"
#include "Dice/hypertrie/internal/util/SwitchTemplateFunctions.hpp"
#include <fmt/format.h>
#include <memory>


namespace hypertrie {

	template<HypertrieTrait tr = default_bool_Hypertrie_t>
	class HypertrieContext {
	private:
		static constexpr const size_t depth_ = hypertrie_depth_limit -1;
	public:
		using NodeContext = typename internal::raw::template NodeContext<depth_, typename internal::raw::template Hypertrie_internal_t<tr>>;

	public:
		NodeContext raw_context{};

	public:

		HypertrieContext(){}

		constexpr size_t depth() const {
			return depth_;
		}
		NodeContext &rawContext() {
			return raw_context;
		}
	};

	template<HypertrieTrait tr>
	class DefaultHypertrieContext {
	public:
		static HypertrieContext<tr>& instance(){
			static const std::unique_ptr<HypertrieContext<tr>> instance{new HypertrieContext<tr>()};
			return *instance;
		}

		DefaultHypertrieContext(const DefaultHypertrieContext&) = delete;
		DefaultHypertrieContext& operator= (const DefaultHypertrieContext) = delete;

	protected:
		DefaultHypertrieContext() {}
	};
}

#endif //HYPERTRIE_HYPERTRIECONTEXT_HPP
