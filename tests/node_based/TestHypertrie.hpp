#ifndef HYPERTRIE_TESTHYPERTRIE_H
#define HYPERTRIE_TESTHYPERTRIE_H

#include <Dice/hypertrie/internal/Hypertrie.hpp>
#include <Dice/hypertrie/internal/HypertrieContext.hpp>


#include <fmt/format.h>

#include "../utils/AssetGenerator.hpp"
#include "../utils/NameOfType.hpp"
#include "../utils/GenerateTriples.hpp"
#include "TestTensor.hpp"


namespace hypertrie::tests::node_context {
	using namespace fmt::literals;
	using namespace hypertrie::tests::utils;

	using namespace hypertrie::internal;



	template<int depth>
	void test_single_write() {
		using tr = default_bool_Hypertrie_t;

		using key_part_type = typename tr::key_part_type;
		using value_type = typename tr::value_type;
		SECTION("Boolhypertrie depth {}"_format(depth)) {
			utils::EntryGenerator<depth, key_part_type, value_type> gen{};
			auto keys = gen.keys(500);
			for (const auto &key : keys) {
				HypertrieContext<tr> context;
				Hypertrie<tr> t{depth, context};
				WARN(fmt::format("[ {} ]",fmt::join(key, ", ")));
				t.set(key, true);
				REQUIRE(t[key]);
			}
		}
	}

	TEST_CASE("test_single_write_read", "[BoolHypertrie]") {
		utils::resetDefaultRandomNumberGenerator();
		test_single_write<1>();
		test_single_write<2>();
		test_single_write<3>();
		test_single_write<4>();
		test_single_write<5>();
	}

	TEST_CASE("test_iterator", "[BoolHypertrie]") {
		using tr = default_bool_Hypertrie_t;
		constexpr const size_t depth = 4;
		using key_part_type = typename tr::key_part_type;
		using value_type = typename tr::value_type;

		utils::EntryGenerator<depth, key_part_type, value_type,1,15> gen{};
		auto keys = gen.keys(150);

//		std::cout << keys << std::endl;

		HypertrieContext<tr> context;
		Hypertrie<tr> t{depth, context};
		for (const auto &key : keys) {
//			WARN(fmt::format("[ {} ]",fmt::join(key, ", ")));
			t.set(key, true);
			REQUIRE(t[key]);
		}

		std::vector<typename tr::Key> actual_keys;
		for (const auto &key : t) {
//			WARN(fmt::format("[ {} ]",fmt::join(key, ", ")));
			actual_keys.push_back(key);
		}

		REQUIRE(keys.size() == actual_keys.size());

		for (const auto &actual_key : actual_keys) {
			REQUIRE(keys.count(actual_key));
		}

	}

	TEST_CASE("test_diagonal", "[BoolHypertrie]") {
		using tr = default_bool_Hypertrie_t;
		constexpr const size_t depth = 4;
		using key_part_type = typename tr::key_part_type;
		using value_type = typename tr::value_type;

		utils::EntryGenerator<depth, key_part_type, value_type,1,15> gen{};
		auto keys = gen.keys(150);

//		std::cout << keys << std::endl;
		{
		HypertrieContext<tr> context;
		Hypertrie<tr> t{depth, context};
			t.set({1,2,3,4}, true);
			HashDiagonal d{t,{2}};
			d.begin();
			REQUIRE(d != d.end());
			std::cout << d.currentKeyPart() <<" → " << std::string(d.currentHypertrie())  << std::endl;
			++d;
			REQUIRE(d == d.end());
		}

		{
			HypertrieContext<tr> context;
			Hypertrie<tr> t{depth, context};
			t.set({1,5,3,4}, true);
			t.set({1,2,3,4}, true);
			HashDiagonal d{t,{1}};
			d.begin();
			REQUIRE(d != d.end());
			std::cout << d.currentKeyPart() <<" → " << std::string(d.currentHypertrie())  << std::endl;
			++d;
			REQUIRE(d != d.end());
			std::cout << d.currentKeyPart() <<" → " << std::string(d.currentHypertrie())  << std::endl;
			++d;
			REQUIRE(d == d.end());
		}

		{
			HypertrieContext<tr> context;
			Hypertrie<tr> t{depth, context};
			t.set({1,5,3,4}, true);
			t.set({1,2,3,4}, true);
			HashDiagonal d{t,{3}};
			d.begin();
			REQUIRE(d != d.end());
			std::cout << d.currentKeyPart() <<" → " << std::string(d.currentHypertrie())  << std::endl;
			++d;
			REQUIRE(d == d.end());
		}

	}

	TEST_CASE("test_slice", "[BoolHypertrie]") {
		using tr = default_bool_Hypertrie_t;
		constexpr const size_t depth = 4;
//		using key_part_type = typename tr::key_part_type;

//		std::cout << keys << std::endl;

		HypertrieContext<tr> context;
		Hypertrie<tr> t{depth, context};
//			WARN(fmt::format("[ {} ]",fmt::join(key, ", ")));
		t.set({1,2,3,4}, true);
		t.set({1,5,6,7}, true);
		t.set({1,8,9,10}, true);
		WARN((std::string) t);
		
		typename tr::SliceKey slice_key = {1, 2, 3, {}};

		const std::variant<std::optional<const_Hypertrie<tr>>, bool> &result = t[slice_key];

		const_Hypertrie sliced_hypertrie = std::get<0>(result).value();

		WARN((std::string) sliced_hypertrie);

	}

};// namespace hypertrie::tests::node_context

#endif//HYPERTRIE_TESTHYPERTRIE_H
