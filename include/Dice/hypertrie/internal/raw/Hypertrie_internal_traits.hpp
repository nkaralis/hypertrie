#ifndef HYPERTRIE_HYPERTRIE_INTERNAL_TRAITS_HPP
#define HYPERTRIE_HYPERTRIE_INTERNAL_TRAITS_HPP

#include <bitset>

#include "Dice/hypertrie/internal/Hypertrie_traits.hpp"
#include "Dice/hypertrie/internal/util/RawKey.hpp"

namespace hypertrie::internal::raw {

	template<HypertrieTrait tr_t = default_bool_Hypertrie_t>
	struct Hypertrie_internal_t {
		using tr = tr_t;
		/// public definitions
		using key_part_type = typename tr::key_part_type;
		using value_type = typename tr::value_type;
		template<typename key, typename value>
		using map_type = typename tr::template map_type<key, value>;
		template<typename key>
		using set_type = typename tr::template set_type<key>;

		using SliceKey = typename tr::SliceKey;
		using Key = typename tr::Key;
		/// internal definitions
		template<size_t depth>
		using RawKey = hypertrie::internal::RawKey<depth, typename tr::key_part_type>;

		static size_t sliceKeyDepth(const SliceKey &slice_key) {
			size_t fixed_depth = 0;
			for (auto opt_key_part : slice_key) {
				if (opt_key_part.has_value())
					++fixed_depth;
			}
			return fixed_depth;
		}

		template<size_t fixed_depth>
		class RawSliceKey {
		public:
			struct FixedValue {
				size_t pos;
				key_part_type key_part;
			};

		private:
			std::array<FixedValue, fixed_depth> fixed_values;

		public:
			explicit RawSliceKey(const SliceKey &slice_key) {
				assert(sliceKeyDepth(slice_key) == fixed_depth);
				size_t pos = 0;
				size_t key_pos = 0;
				for (const auto &opt_key_part : slice_key) {
					if (opt_key_part.has_value())
						fixed_values[pos++] = {key_pos, opt_key_part.value()};
					++key_pos;
				}
			}

			const FixedValue &operator[](size_t pos) const { return fixed_values[pos]; }

			auto begin() const { return fixed_values.cbegin(); }

			auto end() const { return fixed_values.cend(); }
		};

		// TODO: rename to key_positions
		template<size_t depth>
		using DiagonalPositions = std::bitset<depth>;

		constexpr static bool is_bool_valued = tr::is_bool_valued;
		constexpr static const bool is_lsb_unused = tr::lsb_unused;
		constexpr static bool is_tsl_map = std::is_same_v<map_type<int, int>, container::tsl_sparse_map<int, int>>;

		/**
		 * Generates a subkey by removing a key_part at the given position
		 * @tparam depth length of the key
		 * @param key the key
		 * @param remove_pos position to be removed from key
		 * @return a new subkey
		 */
		template<size_t depth>
		static auto subkey(const RawKey<depth> &key, size_t remove_pos) -> RawKey<depth - 1> {
			RawKey<depth - 1> sub_key;
			for (size_t i = 0, j = 0; i < depth; ++i)
				if (i != remove_pos) sub_key[j++] = key[i];
			return sub_key;
		}

		/**
		 * Different maps use different methods to provide access to non-constant mapped_type references. This method provides an abstraction.
		 * @tparam K key_type of the map
		 * @tparam V mapped_type of the map
		 * @param map_it a valid iterator pointing to an entry
		 * @return a reference to the mapped_type
		 */
		template<typename K, typename V>
		static V &deref(typename map_type<K, V>::iterator &map_it) {
			if constexpr (is_tsl_map) return map_it.value();
			else
				return map_it->second;
		}
	};

	namespace internal::hypertrie_internal_trait {
		template<typename T, template<HypertrieTrait> typename U>
		struct is_instance_impl : public std::false_type {
		};

		template<template<HypertrieTrait> typename U,
				 HypertrieTrait tr_t>
		struct is_instance_impl<U<tr_t>, U> : public std::true_type {
		};

		template<typename T, template<HypertrieTrait> typename U>
		using is_instance = is_instance_impl<std::decay_t<T>, U>;
	}// namespace internal::hypertrie_internal_trait


	template<class T>
	concept HypertrieInternalTrait = internal::hypertrie_internal_trait::is_instance<T, Hypertrie_internal_t>::value;

	using default_bool_Hypertrie_internal_t = Hypertrie_internal_t<default_bool_Hypertrie_t>;

	using default_long_Hypertrie_internal_t = Hypertrie_internal_t<default_long_Hypertrie_t>;

	using default_double_Hypertrie_internal_t = Hypertrie_internal_t<default_double_Hypertrie_t>;
};// namespace hypertrie::internal

#endif//HYPERTRIE_HYPERTRIE_INTERNAL_TRAITS_HPP
