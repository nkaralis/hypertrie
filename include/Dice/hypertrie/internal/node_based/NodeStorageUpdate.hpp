#ifndef HYPERTRIE_NODESTORAGEUPDATE_HPP
#define HYPERTRIE_NODESTORAGEUPDATE_HPP

#include "Dice/hypertrie/internal/node_based/Hypertrie_traits.hpp"
#include "Dice/hypertrie/internal/node_based/NodeContainer.hpp"
#include "Dice/hypertrie/internal/node_based/NodeStorage.hpp"
#include "Dice/hypertrie/internal/node_based/TaggedNodeHash.hpp"
#include "Dice/hypertrie/internal/util/CONSTANTS.hpp"
#include <Dice/hypertrie/internal/util/CountDownNTuple.hpp>

#include <tsl/hopscotch_set.h>
#include <tsl/hopscotch_map.h>

namespace hypertrie::internal::node_based {

	template<size_t node_storage_depth,
			 size_t update_depth,
			 HypertrieInternalTrait tri_t = Hypertrie_internal_t<>,
			 typename = typename std::enable_if_t<(node_storage_depth >= 1)>>
	class NodeStorageUpdate {
	public:
		using tri = tri_t;
		/// public definitions
		using key_part_type = typename tri::key_part_type;
		using value_type = typename tri::value_type;
		template<typename key, typename value>
		using map_type = typename tri::template map_type<key, value>;
		template<typename key>
		using set_type = typename tri::template set_type<key>;
		template<size_t depth>
		using RawKey = typename tri::template RawKey<depth>;

		template<size_t depth>
		using RawSliceKey = typename tri::template RawSliceKey<depth>;

		template<size_t depth>
		using NodeStorage_t = NodeStorage<depth, tri>;

	private:
		static const constexpr long INC_COUNT_DIFF_AFTER = 1;
		static const constexpr long DEC_COUNT_DIFF_AFTER = -1;
		static const constexpr long INC_COUNT_DIFF_BEFORE = -1;
		static const constexpr long DEC_COUNT_DIFF_BEFORE = 1;

		template<size_t depth>
		class MultiUpdate;

		template<size_t depth>
		using LevelMultiUpdates_t = std::unordered_set<MultiUpdate<depth>, absl::Hash<MultiUpdate<depth>>>;

		using PlannedMultiUpdates = util::CountDownNTuple<LevelMultiUpdates_t, update_depth>;

		using LevelRefChanges = std::unordered_map<TaggedNodeHash, long>;

		using RefChanges = std::array<LevelRefChanges, update_depth>;

		template<size_t depth>
		class BoolEntry {

			RawKey<depth> key_;

		public:
			BoolEntry() : key_{}{}
			BoolEntry(RawKey<depth> key, [[maybe_unused]] value_type value = value_type(1)) : key_(key) {}


			constexpr value_type value() const noexcept { return value_type(1); };
		};

		template<size_t depth>
		class NonBoolEntry : public BoolEntry<depth> {
			RawKey<depth> key_;

			value_type value_;

		public:
			NonBoolEntry() : key_{},  value_{}{}
			NonBoolEntry(const RawKey<depth> &key, [[maybe_unused]] value_type value = value_type(1)) : key_{key}, value_(value) {}

			key_part_type & operator[](size_t pos) {
				return key_[pos];
			}

			const key_part_type & operator[](size_t pos) const {
				return key_[pos];
			}

			friend class NodeStorageUpdate;
		};

		template<size_t depth>
		static RawKey<depth> &key(NonBoolEntry<depth> &entry){
			return entry.key_;
		}

		template<size_t depth>
		static const RawKey<depth> &key(const NonBoolEntry<depth> &entry){
			return entry.key_;
		}

		template<size_t depth>
		static value_type &value(NonBoolEntry<depth> &entry){
			return entry.value_;
		}

		template<size_t depth>
		static const value_type &value(const NonBoolEntry<depth> &entry){
			return entry.value_;
		}

		template<size_t depth>
		static value_type value([[maybe_unused]] const RawKey<depth> &entry){
			return true;
		}

		template<size_t depth>
		static RawKey<depth> &key([[maybe_unused]] RawKey<depth> &entry){
			return entry;
		}

		template<size_t depth>
		static const RawKey<depth> &key([[maybe_unused]] const RawKey<depth> &entry){
			return entry;
		}

		template<size_t depth>
		using Entry = std::conditional_t <(tri::is_bool_valued), RawKey<depth>, NonBoolEntry<depth>>;


		enum struct InsertOp : unsigned int {
			NONE = 0,
			CHANGE_VALUE,
			INSERT_C_NODE,
			REMOVE_FROM_UC,
			INSERT_MULT_INTO_C,
			INSERT_MULT_INTO_UC,
			NEW_MULT_UC
		};

		template<size_t depth>
		class MultiUpdate {
			static_assert(depth >= 1);


			InsertOp insert_op_{};
			TaggedNodeHash hash_before_{};
			mutable TaggedNodeHash hash_after_{};
			mutable std::vector<Entry<depth>> entries_{};

		public:

			InsertOp &insertOp()  noexcept { return this->insert_op_;}

			const InsertOp &insertOp() const noexcept { return this->insert_op_;}

			TaggedNodeHash &hashBefore()  noexcept { return this->hash_before_;}

			const TaggedNodeHash &hashBefore() const noexcept {
				if (hash_after_.empty()) calcHashAfter();
				return this->hash_before_;
			}

			TaggedNodeHash &hashAfter() noexcept {
				if (hash_after_.empty()) calcHashAfter();
				return this->hash_after_;
			}

			const TaggedNodeHash &hashAfter() const noexcept { return this->hash_after_;}

			std::vector<Entry<depth>> &entries() noexcept { return this->entries_;}

			const std::vector<Entry<depth>> &entries() const noexcept { return this->entries_;}

			void addEntry(Entry<depth> entry) noexcept {
				entries_.push_back(entry);
			}

			void addKey(RawKey<depth> key) noexcept {
				entries_.push_back({key});
			}

			value_type &oldValue() noexcept {
				if constexpr (not tri::is_bool_valued){
					assert(insert_op_ == InsertOp::CHANGE_VALUE);
					entries_.resize(2);
					return value(entries_[1]);
				} else
					assert(false);
			}

			const value_type &oldValue() const noexcept {
				if constexpr (not tri::is_bool_valued){
					assert(insert_op_ == InsertOp::CHANGE_VALUE);
					entries_.resize(2);
					return value(entries_[1]);
				} else
					assert(false);
			}

			RawKey<depth> &firstKey() noexcept { return key(entries_[0]); }

			const RawKey<depth> &firstKey() const noexcept { return key(entries_[0]); }

			value_type &firstValue() noexcept  { return value(entries_[0]); }

			const value_type &firstValue() const noexcept  { return value(entries_[0]); }

		private:
			void calcHashAfter() const noexcept {
				hash_after_ = hash_before_;
				switch (insert_op_) {
					case InsertOp::CHANGE_VALUE:
						assert(not hash_before_.empty());
						assert(entries_.size() == 2);
						this->hash_after_.changeValue(firstKey(), oldValue(), firstValue());
						break;
					case InsertOp::INSERT_C_NODE:
						assert(hash_before_.empty());
						hash_after_ = TaggedNodeHash::getCompressedNodeHash(firstKey(), firstValue());
						break;
					case InsertOp::INSERT_MULT_INTO_C:
						[[fallthrough]];
					case InsertOp::INSERT_MULT_INTO_UC:
						assert(not hash_before_.empty());
						for (const auto &entry : entries_)
							hash_after_.addEntry(key(entry), value(entry));
						break;
					case InsertOp::NEW_MULT_UC:
						assert(hash_before_.empty());
						assert(entries_.size() > 1);

						hash_after_ = TaggedNodeHash::getCompressedNodeHash(firstKey(), firstValue());
						for (auto entry_it = std::next(entries_.begin()); entry_it != entries_.end(); ++entry_it)
							hash_after_.addEntry(key(*entry_it), value(*entry_it));
						break;
					default:
						assert(false);
				}
			}

		public:

			bool operator<(const MultiUpdate<depth> &other) const noexcept {
				return std::make_tuple(this->insert_op_, this->hash_before_, this->hashAfter()) <
					   std::make_tuple(other.insert_op_, other.hash_before_, other.hashAfter());
			};

			bool operator==(const MultiUpdate<depth> &other) const noexcept {
				return std::make_tuple(this->insert_op_, this->hash_before_, this->hashAfter()) ==
					   std::make_tuple(other.insert_op_, other.hash_before_, other.hashAfter());
			};

			template<typename H>
			friend H AbslHashValue(H h, const MultiUpdate<depth> &update) {
				return H::combine(std::move(h), update.hash_before_, update.hashAfter());
			}
		};

	public:
		NodeStorage_t<node_storage_depth> &node_storage;

		UncompressedNodeContainer<update_depth, tri> &nodec;

		PlannedMultiUpdates planned_multi_updates{};

		RefChanges ref_changes{};

		template<size_t updates_depth>
		auto getRefChanges()
				-> LevelRefChanges & {
			return ref_changes[updates_depth - 1];
		}

		// extract a change from count_changes
		template<size_t updates_depth>
		void planChangeCount(const TaggedNodeHash hash, const long count_change) {
			LevelRefChanges &count_changes = getRefChanges<updates_depth>();
			count_changes[hash] += count_change;
		};


		template<size_t updates_depth>
		auto getPlannedMultiUpdates()
				-> LevelMultiUpdates_t<updates_depth> & {
			return std::get<updates_depth - 1>(planned_multi_updates);
		}

		template<size_t updates_depth>
		void planUpdate(MultiUpdate<updates_depth> planned_update, const long count_diff) {
			auto &planned_updates = getPlannedMultiUpdates<updates_depth>();
			if (not planned_update.hashBefore().empty())
				planChangeCount<updates_depth>(planned_update.hashBefore(), -1 * count_diff);
			planChangeCount<updates_depth>(planned_update.hashAfter(), count_diff);
			planned_updates.insert(std::move(planned_update));
		}


		NodeStorageUpdate(NodeStorage_t<node_storage_depth> &nodeStorage, UncompressedNodeContainer<update_depth, tri> &nodec)
			: node_storage(nodeStorage), nodec{nodec} {}

		auto apply_update(std::vector<RawKey<update_depth>> keys) {
			assert(keys.size() > 2);
			MultiUpdate<update_depth> update{};
			update.insertOp() = InsertOp::INSERT_MULT_INTO_UC;
			update.hashBefore() = nodec.thash_;
			update.entries() = std::move(keys);

			planUpdate(std::move(update), INC_COUNT_DIFF_AFTER);

			apply_update_rek<update_depth>();
		}

		void apply_update(const RawKey<update_depth> &key, const value_type value, const value_type old_value) {
			if (value == old_value)
				return;

			bool value_deleted = value == value_type{};

			bool value_changes = old_value != value_type{};

			MultiUpdate<update_depth> update{};
			update.hashBefore() = nodec;
			update.addEntry({key, value});
			if (value_deleted) {
				update.insertOp() = InsertOp::REMOVE_FROM_UC;
				throw std::logic_error{"deleting values from hypertrie is not yet implemented. "};
			} else if (value_changes) {
				update.insertOp() = InsertOp::CHANGE_VALUE;
				update.oldValue() = old_value;
			} else {// new entry
				update.insertOp() = InsertOp::INSERT_MULT_INTO_UC;
			}
			planUpdate(std::move(update), INC_COUNT_DIFF_AFTER);
			apply_update_rek<update_depth>();
		}

		template<size_t depth>
		void apply_update_rek() {

			LevelMultiUpdates_t<depth> &multi_updates = getPlannedMultiUpdates<depth>();

			// nodes_before that are have ref_count 0 afterwards -> those could be reused/moved for nodes after
			tsl::hopscotch_set<TaggedNodeHash> unreferenced_nodes_before{}; // TODO: store the pointer alongside to safe one resolve

			LevelRefChanges new_after_nodes{};

			// extract a change from count_changes
			auto pop_after_count_change = [&](TaggedNodeHash hash) -> std::tuple<bool, long> {
				if (auto changed = new_after_nodes.find(hash); changed != new_after_nodes.end()) {
					auto diff = changed->second;
					new_after_nodes.erase(changed);
				  return {diff != 0, diff};
			  } else
				  return {false, 0};
			};


			// check if a hash is in count_changes
			auto peak_after_count_change = [&](TaggedNodeHash hash) -> bool {
			  if (auto changed = new_after_nodes.find(hash); changed != new_after_nodes.end()) {
				  auto diff = changed->second;
				  return diff != 0;
			  } else
				  return false;
			};

			// process reference changes to nodes_before
			for (const auto &[hash, count_diff] : getRefChanges<depth>()) {
				assert(not hash.empty());
				if (count_diff == 0)
					continue;
				auto nodec = node_storage.template getNode<depth>(hash);
				if (not nodec.null()){
					assert(not nodec.null());
					size_t *ref_count = [&]() {
						if (nodec.isCompressed())
							return &nodec.compressed_node()->ref_count();
						else
							return &nodec.uncompressed_node()->ref_count();
					}();
					*ref_count += count_diff;

					if (*ref_count == 0) {
						unreferenced_nodes_before.insert(hash);
					}
				} else {
					new_after_nodes.insert({hash, count_diff});
				}
			}

			static std::vector<std::pair<MultiUpdate<depth>, long>> moveable_multi_updates{};
			moveable_multi_updates.clear();
			moveable_multi_updates.reserve(multi_updates.size());
			// extract movables
			for (const MultiUpdate<depth> &update : multi_updates) {
				// skip if it cannot be a movable
				if (update.insertOp() == InsertOp::NEW_MULT_UC
					or update.insertOp() == InsertOp::INSERT_C_NODE
					or update.insertOp() ==  InsertOp::INSERT_MULT_INTO_C)
					continue;
				// check if it is a moveable. then save it and skip to the next iteration
				auto unref_before = unreferenced_nodes_before.find(update.hashBefore());
				if (unref_before != unreferenced_nodes_before.end()) {
					if (peak_after_count_change(update.hashAfter())) {
						unreferenced_nodes_before.erase(unref_before);
						auto [changes, after_count_change] = pop_after_count_change(update.hashAfter());
						// TODO: move the keys here
						moveable_multi_updates.emplace_back(update, after_count_change);
					}
				}
			}

			static std::vector<std::pair<MultiUpdate<depth>, long>> unmoveable_multi_updates{};
			unmoveable_multi_updates.clear();
			unmoveable_multi_updates.reserve(multi_updates.size());
			// extract unmovables
			for (const MultiUpdate<depth> &update : multi_updates) {
				// check if it is a moveable. then save it and skip to the next iteration
				auto [changes, after_count_change] = pop_after_count_change(update.hashAfter());
				if (changes) {
					// TODO: move the keys here
					unmoveable_multi_updates.emplace_back(update, after_count_change);
				}
			}

			assert(new_after_nodes.empty());

			std::unordered_map<TaggedNodeHash, long> node_before_children_count_diffs{};

			// do unmoveables
			for (auto &[update, count_change] : unmoveable_multi_updates) {
				assert (count_change > 0);
				long node_before_children_count_diff;
				if (update.hashBefore().isCompressed())
					node_before_children_count_diff = processUpdate<depth, NodeCompression::compressed, false>(update, (size_t)count_change);
				else
					node_before_children_count_diff = processUpdate<depth, NodeCompression::uncompressed, false>(update, count_change);

				if (node_before_children_count_diff != 0)
					node_before_children_count_diffs[update.hashBefore()] += node_before_children_count_diff;
			}

			// remove remaining unreferenced_nodes_before and update the references of their children
			for (const auto &hash_before : unreferenced_nodes_before) {
				if (hash_before.isCompressed()){
					removeUnreferencedNode<depth, NodeCompression::compressed>(hash_before);
				} else {
					auto handle = node_before_children_count_diffs.extract(hash_before);
					long children_count_diff = DEC_COUNT_DIFF_BEFORE;
					if (not handle.empty())
						children_count_diff += handle.mapped();

					removeUnreferencedNode<depth, NodeCompression::uncompressed>(hash_before, children_count_diff);
				}
			}

			// update the references of children not covered above (children of nodes that were not removed)
			for (const auto &[hash_before, children_count_diff] : node_before_children_count_diffs) {
				updateChildrenCountDiff<depth>(hash_before, children_count_diff);
			}

			// do moveables
			for (auto &[update, count_change] : moveable_multi_updates) {
				if (update.hashBefore().isCompressed())
					processUpdate<depth, NodeCompression::compressed, true>(update, count_change);
				else
					processUpdate<depth, NodeCompression::uncompressed, true>(update, count_change);
			}

			if constexpr (depth > 1)
				apply_update_rek<depth - 1>();
		}

		template<size_t depth>
		void updateChildrenCountDiff(const TaggedNodeHash hash,const long children_count_diff){
			assert(hash.isUncompressed());
			if constexpr (depth > 1){
				if (children_count_diff != 0) {
					auto node = node_storage.template getUncompressedNode<depth>(hash).node();
					for (size_t pos : iter::range(depth)) {
						for (const auto &[_, child_hash] : node->edges(pos)) {
							planChangeCount<depth -1 >(child_hash, -1*children_count_diff);
						}
					}
				}
			}
		}

		template<size_t depth, NodeCompression compression>
		void removeUnreferencedNode(const TaggedNodeHash hash, long children_count_diff = 0){
			if constexpr(compression == NodeCompression::uncompressed)
				this->template updateChildrenCountDiff<depth>(hash, children_count_diff);

			node_storage.template deleteNode<depth>(hash);
		}

		template<size_t depth, NodeCompression compression, bool reuse_node_before = false>
		long processUpdate(MultiUpdate<depth> &update, const size_t after_count_diff) {
			auto nc_after = node_storage.template getNode<depth>(update.hashAfter());
			assert(nc_after.null());

			long node_before_children_count_diff = 0;
			switch (update.insertOp()) {
				case InsertOp::CHANGE_VALUE:
					node_before_children_count_diff = changeValue<depth, compression, reuse_node_before>(update, after_count_diff);
					break;
				case InsertOp::INSERT_C_NODE:
					assert(compression == NodeCompression::uncompressed);
					if constexpr(compression == NodeCompression::uncompressed)
						insertCompressedNode<depth>(update, after_count_diff);
					break;
				case InsertOp::INSERT_MULT_INTO_UC:
					if constexpr (compression == NodeCompression::uncompressed)
						node_before_children_count_diff = insertBulkIntoUC<depth, reuse_node_before>(update, after_count_diff);
					break;
				case InsertOp::INSERT_MULT_INTO_C:
					assert(compression == NodeCompression::compressed);
					if constexpr (compression == NodeCompression::compressed)
						insertBulkIntoC<depth>(update, after_count_diff);
					break;
				case InsertOp::NEW_MULT_UC:
					assert(compression == NodeCompression::uncompressed);
					if constexpr (compression == NodeCompression::uncompressed)
						newUncompressedBulk<depth>(update, after_count_diff);
					break;
				case InsertOp::REMOVE_FROM_UC:
					assert(false);// not yet implemented
					break;
				default:
					assert(false);
			}
			return node_before_children_count_diff;
		}

		template<size_t depth>
		void insertCompressedNode(const MultiUpdate<depth> &update, const size_t after_count_diff) {

			node_storage.template newCompressedNode<depth>(
					update.firstKey(), update.firstValue(), after_count_diff, update.hashAfter());
		}

		template<size_t depth, NodeCompression compression, bool reuse_node_before = false>
		long changeValue(const MultiUpdate<depth> &update, const size_t after_count_diff) {
			long node_before_children_count_diff = 0;
			SpecificNodeContainer<depth, compression, tri> nc_before = node_storage.template getNode<depth, compression>(update.hashBefore());
			assert(not nc_before.null());

			SpecificNodeContainer<depth, compression, tri> nc_after;

			// reusing the node_before
			if constexpr (reuse_node_before) {// node before ref_count is zero -> maybe reused

				// create updates for sub node
				if constexpr (compression == NodeCompression::uncompressed) {
					UncompressedNode <depth, tri> *node = nc_before.node();


					if constexpr (depth > 1) {
						// change the values of children recursively


						static constexpr const auto subkey = &tri::template subkey<depth>;
						for (const size_t pos : iter::range(depth)) {
							auto sub_key = subkey(update.firstKey(), pos);
							auto key_part = update.firstKey()[pos];

							MultiUpdate<depth - 1> child_update{};
							child_update.insertOp() = InsertOp::CHANGE_VALUE;
							child_update.oldValue() = update.oldValue();
							child_update.addEntry({sub_key, update.firstValue()});

							child_update.hashBefore() = node->child(pos, key_part);
							assert(not child_update.hashBefore().empty());

							planUpdate(std::move(child_update), INC_COUNT_DIFF_AFTER);
						}
					}
				}

				// update the node_before with the after_count and value
				nc_after = node_storage.template changeNodeValue<depth, compression, false>(
						nc_before, update.firstKey(), update.oldValue(), update.firstValue(), after_count_diff, update.hashAfter());

			} else {// leaving the node_before untouched
				if constexpr (compression == NodeCompression::compressed)
					node_storage.template newCompressedNode<depth>( // TODO: use firstKey
							nc_before.node()->key(), update.firstValue(), after_count_diff, update.hashAfter());
				else {
					if constexpr (depth > 1) {
						node_before_children_count_diff = INC_COUNT_DIFF_BEFORE;
					}

					if constexpr (compression == NodeCompression::uncompressed and depth > 1) {
						auto *node = nc_before.node();

						static constexpr const auto subkey = &tri::template subkey<depth>;
						for (const size_t pos : iter::range(depth)) {
							auto sub_key = subkey(update.firstKey(), pos);
							auto key_part = update.firstKey()[pos];

							MultiUpdate<depth - 1> child_update{};
							child_update.insertOp() = InsertOp::CHANGE_VALUE;
							child_update.oldValue() = update.oldValue();
							child_update.addEntry({sub_key, update.firstValue()});

							child_update.hashBefore() = node->child(pos, key_part);
							assert(not child_update.hashBefore().empty());

							planUpdate(std::move(child_update), INC_COUNT_DIFF_AFTER);
						}
					}

					nc_after = node_storage.template changeNodeValue<depth, compression, true>(
							nc_before, update.firstKey(), update.oldValue(), update.firstValue(), after_count_diff, update.hashAfter());
				}
			}

			if constexpr (depth == update_depth and compression == NodeCompression::uncompressed)
				this->nodec = nc_after;

			return node_before_children_count_diff;
		}

		template<size_t depth>
		void newUncompressedBulk(const MultiUpdate<depth> &update, const size_t after_count_diff) {
			// TODO: implement for valued
			if constexpr (not tri::is_bool_valued)
				return;
			else {
				static constexpr const auto subkey = &tri::template subkey<depth>;

				// move or copy the node from old_hash to new_hash
				auto &storage = node_storage.template getNodeStorage<depth, NodeCompression::uncompressed>();
				// make sure everything is set correctly
				assert(update.hashBefore().empty());
				assert(storage.find(update.hashAfter()) == storage.end());

				// create node and insert it into the storage
				UncompressedNode<depth, tri> *const node = new UncompressedNode<depth, tri>{(size_t) after_count_diff};
				storage.insert({update.hashAfter(), node});

				if constexpr (depth > 1)
					node->size_ = update.keys.size();

				// populate the new node
				for (const size_t pos : iter::range(depth)) {
					if constexpr (depth == 1) {
						for (const RawKey<depth> &key : update.keys)
							node->edges().insert(key[0]);
					} else {
						// # group the subkeys by the key part at pos

						// maps key parts to the keys to be inserted for that child
						std::unordered_map<key_part_type, std::vector<RawKey<depth - 1>>> children_inserted_keys{};

						// populate children_inserted_keys
						for (const RawKey<depth> &key : update.keys)
							children_inserted_keys[key[pos]].push_back(subkey(key, pos));

						// process the changes to the node at pos and plan the updates to the sub nodes
						for (auto &[key_part, child_inserted_keys] : children_inserted_keys) {
							assert(child_inserted_keys.size() > 0);
							auto [key_part_exists, iter] = node->find(pos, key_part);

							TaggedNodeHash hash_after;
							// plan the changes and calculate hash-after
							const TaggedNodeHash child_hash = (key_part_exists) ? iter->second : TaggedNodeHash{};
							// EXPAND_C_NODE (insert only one key)
							if (child_inserted_keys.size() == 1) {
								// plan next update
								MultiUpdate<depth - 1> child_update{};
								child_update.key = child_inserted_keys[0];
								child_update.value = true;
								child_update.insert_op = InsertOp::INSERT_C_NODE;
								child_update.calcHashAfter();

								// safe hash_after for executing the change
								hash_after = child_update.hashAfter();
								// submit the planned update
								planUpdate(std::move(child_update), 1);
							} else if (child_inserted_keys.size() == 2) {
								MultiUpdate<depth - 1> child_update{};
								child_update.key = child_inserted_keys[0];
								child_update.value = true;
								child_update.second_key = child_inserted_keys[1];
								child_update.second_value = true;
								child_update.insert_op = InsertOp::INSERT_TWO_KEY_UC_NODE;
								child_update.calcHashAfter();

								// safe hash_after for executing the change
								hash_after = child_update.hashAfter();

								planUpdate(std::move(child_update), 1);
							} else {

								MultiUpdate<depth - 1> child_update(InsertOp::NEW_MULT_UC, child_hash);
								child_update.keys = std::move(child_inserted_keys);

								child_update.calcHashAfter();

								// safe hash_after for executing the change
								hash_after = child_update.hashAfter();
								planUpdate(std::move(child_update), 1);
							}
							// execute changes
							node->edges(pos)[key_part] = hash_after;
						}
					}
				}
			}
		}

		template<size_t depth>
		void insertBulkIntoC(MultiUpdate<depth> &update, const long after_count_diff) {
			auto &storage = node_storage.template getNodeStorage<depth, NodeCompression::compressed>();
			assert(storage.find(update.hashBefore()) != storage.end());
			assert(storage.find(update.hashAfter()) == storage.end());

			CompressedNode<depth, tri> const *const node_before = storage[update.hashBefore()];

			update.insertOp() = InsertOp::NEW_MULT_UC;
			update.addKey(node_before->key());
			update.hashBefore() = {};
			newUncompressedBulk<depth>(update, after_count_diff);
		}

		template<size_t depth, bool reuse_node_before = false>
		long insertBulkIntoUC(const MultiUpdate<depth> &update, const long after_count_diff) {
			// TODO: implement for valued
			if constexpr (not tri::is_bool_valued)
				return 0;
			else {
				static constexpr const auto subkey = &tri::template subkey<depth>;
				const long node_before_children_count_diff =
						(not reuse_node_before and depth > 1) ? INC_COUNT_DIFF_BEFORE : 0;

				// move or copy the node from old_hash to new_hash
				auto &storage = node_storage.template getNodeStorage<depth, NodeCompression::uncompressed>();
				auto node_it = storage.find(update.hashBefore());
				assert(node_it != storage.end());
				UncompressedNode<depth, tri> *node = node_it->second;
				if constexpr (reuse_node_before) {// node before ref_count is zero -> maybe reused
					storage.erase(node_it);
				} else {
					node = new UncompressedNode<depth, tri>{*node};
					node->ref_count() = 0;
				}
				assert(storage.find(update.hashAfter()) == storage.end());
				storage[update.hashAfter()] = node;

				// update the node count
				node->ref_count() += after_count_diff;
				if constexpr (depth > 1)
					node->size_ += update.entries().size();

				// update the node (new_hash)
				for (const size_t pos : iter::range(depth)) {
					// # group the subkeys by the key part at pos
					if constexpr (depth == 1) {
						for (const RawKey<depth> &key : update.keys)
							node->edges().insert(key[0]);
					} else {
						// maps key parts to the keys to be inserted for that child
						std::unordered_map<key_part_type, std::vector<RawKey<depth - 1>>> children_inserted_keys{};

						// populate children_inserted_keys
						for (const RawKey<depth> &key : update.keys)
							children_inserted_keys[key[pos]].push_back(subkey(key, pos));

						// process the changes to the node at pos and plan the updates to the sub nodes
						for (auto &[key_part, child_inserted_keys] : children_inserted_keys) {
							assert(child_inserted_keys.size() > 0);
							// TODO: handle what happens when we hit level 1
							auto [key_part_exists, iter] = node->find(pos, key_part);

							TaggedNodeHash child_hash_after;
							// plan the changes and calculate hash-after
							const TaggedNodeHash child_hash_before = (key_part_exists) ? iter->second : TaggedNodeHash{};
							// EXPAND_C_NODE (insert only one key)
							if (child_inserted_keys.size() == 1) {
								// plan next update
								MultiUpdate<depth - 1> child_update{};
								child_update.hashBefore() = child_hash_before;
								child_update.key = child_inserted_keys[0];
								child_update.value = true;
								if (key_part_exists) {
									if (child_hash_before.isCompressed())
										child_update.insert_op = InsertOp::EXPAND_C_NODE;
									else
										child_update.insert_op = InsertOp::EXPAND_UC_NODE;
								} else {
									child_update.insert_op = InsertOp::INSERT_C_NODE;
								}
								child_update.calcHashAfter();

								// safe child_hash_after for executing the change
								child_hash_after = child_update.hashAfter();
								// submit the planned update
								planUpdate(std::move(child_update), 1);
							} else {// INSERT_MULT_INTO_UC (insert multiple keys)
								if (not key_part_exists and child_inserted_keys.size() == 2) {
									MultiUpdate<depth - 1> child_update{};
									child_update.key = child_inserted_keys[0];
									child_update.value = true;
									child_update.second_key = child_inserted_keys[1];
									child_update.second_value = true;
									child_update.insert_op = InsertOp::INSERT_TWO_KEY_UC_NODE;
									child_update.calcHashAfter();

									// safe child_hash_after for executing the change
									child_hash_after = child_update.hashAfter();

									planUpdate(std::move(child_update), 1);
								} else {
									InsertOp insert_op;

									if (key_part_exists)
										if (child_hash_before.isCompressed())
											insert_op = InsertOp::INSERT_MULT_INTO_C;
										else
											insert_op = InsertOp::INSERT_MULT_INTO_UC;
									else
										insert_op = InsertOp::NEW_MULT_UC;
									MultiUpdate<depth - 1> child_update(insert_op, child_hash_before);
									child_update.keys = std::move(child_inserted_keys);

									child_update.calcHashAfter();

									// safe child_hash_after for executing the change
									child_hash_after = child_update.hashAfter();
									planUpdate(std::move(child_update), 1);
								}
							}
							// execute changes
							if (key_part_exists)
								tri::template deref<key_part_type, TaggedNodeHash>(iter) = child_hash_after;
							else
								node->edges(pos)[key_part] = child_hash_after;
						}
					}
				}
				if constexpr (depth == update_depth)
					this->nodec.thash_ = update.hashAfter();

				return node_before_children_count_diff;
			}

		}


	};
}// namespace hypertrie::internal::node_based

#endif//HYPERTRIE_NODESTORAGEUPDATE_HPP
