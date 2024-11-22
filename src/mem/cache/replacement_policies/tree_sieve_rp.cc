#include "mem/cache/replacement_policies/tree_sieve_rp.hh"
#include "base/random.hh"
#include "params/TreeSIEVERP.hh"
#include <stdio.h>

namespace gem5 {

namespace replacement_policy {

/**
 * Get the index of the parent of the given indexed subtree.
 *
 * @param Index of the queried tree.
 * @return The index of the parent tree.
 */
static uint64_t parentIndex(const uint64_t index) {
  return std::floor((index - 1) / 2);
}

/**
 * Get index of the subtree on the left of the given indexed tree.
 *
 * @param index The index of the queried tree.
 * @return The index of the subtree to the left of the queried tree.
 */
static uint64_t leftSubtreeIndex(const uint64_t index) { return 2 * index + 1; }

/**
 * Get index of the subtree on the right of the given indexed tree.
 *
 * @param index The index of the queried tree.
 * @return The index of the subtree to the right of the queried tree.
 */
static uint64_t rightSubtreeIndex(const uint64_t index) {
  return 2 * index + 2;
}

/**
 * Find out if the subtree at index corresponds to the right or left subtree
 * of its parent tree.
 *
 * @param index The index of the subtree.
 * @return True if it is a right subtree, false otherwise.
 */
static bool isRightSubtree(const uint64_t index) { return index % 2 == 0; }

TreeSIEVE::TreeSIEVE(const Params &p)
    : Base(p), assoc(p.assoc),
      tree(new SIEVETree(p.assoc > 2 ? (p.assoc / 2) - 1 : 0, false)),
      tree_depth(p.assoc > 2 ? uint64_t(log2(p.assoc)) - 1 : 0),
      num_nodes(p.assoc > 2 ? (p.assoc / 2) - 1 : 0), count(0) {}

// Marks a cache line as invalid
void TreeSIEVE::invalidate(
    const std::shared_ptr<ReplacementData> &replacement_data) {
  std::shared_ptr<TreeSIEVEReplData> repl_data =
      std::static_pointer_cast<TreeSIEVEReplData>(replacement_data);
  repl_data->stale = true;
}

void TreeSIEVE::touch(
    const std::shared_ptr<ReplacementData> &replacement_data) const {
  std::static_pointer_cast<TreeSIEVEReplData>(replacement_data)->stale = false;
}

void TreeSIEVE::reset(
    const std::shared_ptr<ReplacementData> &replacement_data) const {
  std::shared_ptr<TreeSIEVEReplData> repl_data =
      std::static_pointer_cast<TreeSIEVEReplData>(replacement_data);
  repl_data->stale = true;
}

ReplaceableEntry *
TreeSIEVE::getVictim(const ReplacementCandidates &candidates) const {
  // There should be at least one eviction candidate
  assert(candidates.size() > 0);

  if (assoc == 1) {
    return candidates.at(0);
  } else if (assoc == 2) {
    if (candidates.size() < 2) {
      return candidates.at(0);
    }
    std::shared_ptr<TreeSIEVEReplData> left_data =
        std::static_pointer_cast<TreeSIEVEReplData>(
            candidates[0]->replacementData);
    std::shared_ptr<TreeSIEVEReplData> right_data =
        std::static_pointer_cast<TreeSIEVEReplData>(
            candidates[1]->replacementData);
    if ((left_data->stale && right_data->stale) ||
        (!left_data->stale && !right_data->stale)) {
      bool direction = random_mt.random<uint8_t>() % 2 == 0;
      if (direction) {
        return candidates.at(1);
      } else {
        return candidates.at(0);
      }
    } else if (left_data->stale && !right_data->stale) {
      return candidates.at(0);
    } else {
      return candidates.at(1);
    }
  }

  uint64_t trace_ind = 0;

  while (trace_ind < tree->size()) {
    if (tree->at(trace_ind)) {
      trace_ind = rightSubtreeIndex(trace_ind);
    } else {
      trace_ind = leftSubtreeIndex(trace_ind);
    }
  }

  bool evict_direction;
  bool cursor_move;
  uint64_t first_leaf_ind = assoc - 1;
  uint64_t left_ind = trace_ind * 2 + 1 - first_leaf_ind;
  uint64_t right_ind = trace_ind * 2 + 2 - first_leaf_ind;
  uint64_t evict_ind;

  std::shared_ptr<TreeSIEVEReplData> left_data =
      std::static_pointer_cast<TreeSIEVEReplData>(
          candidates.at(left_ind)->replacementData);
  std::shared_ptr<TreeSIEVEReplData> right_data =
      std::static_pointer_cast<TreeSIEVEReplData>(
          candidates.at(right_ind)->replacementData);

  if (!left_data->stale && !right_data->stale) {
    evict_direction = random_mt.random<uint8_t>() % 2 == 0;
    cursor_move = true;
  } else if (!left_data->stale && right_data->stale) {
    evict_direction = true;
    cursor_move = true;
  } else if (left_data->stale && !right_data->stale) {
    evict_direction = false;
    cursor_move = true;
  } else {
    evict_direction = random_mt.random<uint8_t>() % 2 == 0;
    cursor_move = false;
  }

  uint64_t evict_offset = evict_direction ? 1 : 0;
  uint64_t evict_node_ind = trace_ind * 2 + 1 + evict_offset;
  evict_ind = evict_node_ind - first_leaf_ind;
  assert(evict_ind < assoc);

  if (cursor_move) {
    while (trace_ind >= 1) {
      bool is_right = isRightSubtree(trace_ind);
      trace_ind = parentIndex(trace_ind);
      if (is_right) {
        tree->at(trace_ind) = false;
      } else {
        tree->at(trace_ind) = true;
        break;
      }
    }
  }

  return candidates.at(evict_ind);
}

std::shared_ptr<ReplacementData> TreeSIEVE::instantiateEntry() {
  // TreeSIEVEReplData *repl_data = new TreeSIEVEReplData(
  //     (count % assoc) + assoc - 1, std::shared_ptr<SIEVETree>(tree));

  // count++;

  return std::shared_ptr<ReplacementData>(new TreeSIEVEReplData());
}

} // namespace replacement_policy
} // namespace gem5
