#include "mem/cache/replacement_policies/three_tree_rp.hh"

#include <cmath>

#include "params/ThreeTreeRP.hh"

namespace gem5 {

namespace replacement_policy {

ThreeTree::ThreeTreeReplData::ThreeTreeReplData(const int cache_index,
                                                std::vector<bool> *tree, )
    : cache_index(cache_index), tree(tree) {}

ThreeTree::ThreeTree(const Params &p) : Base(p) {
  fatal_if(p.a < 4);

  assoc = p.a;
  obj_count = 0;
  cold_tree = std::vector(assoc);
  prob_tree = std::vector(assoc);
  hot_tree = std::vector(assoc);
  std::fill(cold_tree.begin(), cold_tree.end(), false);
  std::fill(prob_tree.begin(), prob_tree.end(), false);
  std::fill(hot_tree.begin(), hot_tree.end(), false);
}

void ThreeTree::invalidate(
    const std::shared_ptr<ReplacementData> &replacement_data) {
  std::shared_ptr<ThreeTreeReplData> repl_data =
      std::static_pointer_cast<ThreeTreeReplData>(replacement_data);
  int trace_ind = repl_data->tree_index;
  std::vector<bool> *tree = repl_data->tree;
  while (trace_ind > 0) {
    if (trace_ind % 2 == 0) {
      // Right child
      (*tree)[trace_ind] = true;
    } else {
      // Left child
      (*tree)[trace_ind] = false;
    }
    trace_ind = trace_ind / 2;
  }
}

void ThreeTree::touch(
    const std::shared_ptr<ReplacementData> &replacement_data) {
  std::shared_ptr<ThreeTreeReplData> repl_data =
      std::static_pointer_cast<ThreeTreeReplData>(replacement_data);

  std::vector<bool> *subtree = repl_data->tree;
  int tree_index = repl_data->tree_index;

  std::vector<bool> *next_tree = nullptr;
  ThreeTreeReplData **cur_repl_arr = nullptr;
  ThreeTreeReplData **next_repl_arr = nullptr;
  size_t next_tree_size;
  size_t next_tree_depth;
  if (subtree == &cold_tree) {
    next_tree = &prob_tree;
    next_tree_depth = prob_depth;
    next_tree_size = assoc / 4;
    next_repl_arr = cold_repl_arr;
    next_repl_arr = prob_repl_arr;
  } else if (subtree == &prob_tree) {
    next_tree = &hot_tree;
    next_tree_depth = hot_depth;
    next_tree_size = assoc / 2;
    next_repl_arr = prob_repl_arr;
    next_repl_arr = hot_repl_arr;
  }

  std::vector<bool> *touch_tree = &hot_tree;
  size_t touch_ind;
  if (next_tree != nullptr) {
    // Find LRU of next_tree
    size_t trace_ind = 0;
    for (int i = 0; i < next_tree_depth; ++i) {
      if ((*next_tree)[trace_ind]) {
        // Trace right
        trace_ind = trace_ind * 2 + 2;
      } else {
        // Trace left
        trace_ind = trace_ind * 2 + 1;
      }
    }

    touch_ind = trace_ind;
    touch_tree = next_tree;

    size_t next_lru_ind = trace_ind - (next_tree_size - 1);

    // Swap with subtree[tree_index]
    size_t leaf_index = tree_index + 1 - (assoc / 4);
    {
      ThreeTreeReplData temp_repl = *cur_repl_arr[leaf_index];
      *cur_repl_arr[leaf_index] = *next_repl_arr[next_lru_ind];
      *next_repl_arr[next_lru_ind] = temp_repl;
      // Then swap the cache indices back to their original position
      size_t temp_ind = cur_repl_arr[leaf_index]->cache_index;
      cur_repl_arr[leaf_index]->cache_index =
          next_repl_arr[next_lru_ind]->cache_index;
      next_repl_arr[next_lru_ind]->cache_index = temp_ind;
    }
  }

  while (touch_ind > 0) {
    size_t parent_ind = (touch_ind - 1) / 2;
    if (touch_ind % 2 == 0) {
      // Right child
      touch_tree[parent_ind] = false;
    } else {
      // Left child
      touch_tree[parent_ind] = true;
    }
    touch_ind = parent_ind;
  }
}

void ThreeTree::reset(
    const std::shared_ptr<ReplacementData> &replacement_data) {
  touch(replacement_data);
}

ReplaceableEntry *
ThreeTree::getVictim(const ReplacementCandidate &candidates) const {

  size_t tree_choice = rand() % 8;
  size_t trace_ind = 0;
  std::vector<bool> *evict_tree;
  ThreeTreeReplData **repl_arr = nullptr;
  size_t tree_depth;
  bool chose_cold = false, chose_prob = false, chose_hot = false;
  if (tree_choice < 5) {
    // Choose from cold queue
    evict_tree = cold_tree;
    repl_arr = cold_repl_arr;
    tree_depth = cold_depth;
    chose_cold = true;
  } else if (tree_choice < 7) {
    // Choose from probation queue
    evict_tree = prob_tree;
    repl_arr = prob_repl_arr;
    tree_depth = prob_depth;
    chose_prob = true;
  } else {
    // Choose from hot queue;
    evict_tree = hot_tree;
    repl_arr = hot_repl_arr;
    tree_depth = hot_depth;
    chose_hot = true;
  }

  for (int i = 0; i < tree_depth; ++i) {
    if ((*evict_tree)[trace_ind]) {
      // Trace right
      trace_ind = trace_ind * 2 + 2;
    } else {
      // Trace left
      trace_ind = trace_ind * 2 + 1;
    }
  }

  if (chose_cold) {
    size_t evict_ind = trace_ind - (assoc / 4) + 1;
    size_t cache_ind = cold_repl_arr[evict_ind]->cache_index;
    return candidates.at(cache_ind);
  } else if (chose_prob) {
    size_t evict_ind = trace_ind + 1;
    size_t cache_ind = prob_repl_arr[evict_ind]->cache_index;
    return candidates.at(cache_ind);
  } else if (chose_hot) {
    size_t evict_ind = trace_ind + 1;
    size_t cache_ind = hot_repl_arr[evict_ind]->cache_index;
    return candidates.at(cache_ind);
  }
}

std::shared_ptr<ReplacementData> ThreeTree::instantiateEntry() {
  ThreeTreeReplData *repl = new ThreeTreeReplData(obj_count);
  if (count < (assoc / 4)) {
    repl->tree_index = cache_index;
    this->cold_repl_arr[count] = repl;
  } else if (count < (assoc / 2)) {
    repl->tree_index = cache_index - (assoc / 4);
    this->prob_repl_arr[count - 4] = repl;
  } else {
    repl->tree_index = cache_index - (assoc / 2);
    this->hot_repl_arr[count - 8] = repl;
  }
  this->count++;
  return std::shared::ptr<ReplacementData>(repl);
}

} // namespace replacement_policy
} // namespace gem5
