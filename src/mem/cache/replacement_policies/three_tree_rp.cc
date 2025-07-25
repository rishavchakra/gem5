#include "mem/cache/replacement_policies/three_tree_rp.hh"

#include <cmath>
#include <memory>

#include "params/ThreeTreeRP.hh"

namespace gem5 {

namespace replacement_policy {

ThreeTree::ThreeTreeReplData::ThreeTreeReplData(int cache_index)
    : cache_index(cache_index), tree(nullptr), tree_index(0) {}

ThreeTree::ThreeTree(const Params &p) : Base(p) {
  fatal_if(p.a < 4, "3Tree undefined with assoc < 4");

  assoc = p.a;
  obj_count = 0;
  cold_tree = new TTTree(assoc, false);
  prob_tree = new TTTree(assoc, false);
  hot_tree = new TTTree(assoc, false);

  cold_repl_arr = new ThreeTreeReplData *[assoc / 4];
  prob_repl_arr = new ThreeTreeReplData *[assoc / 4];
  hot_repl_arr = new ThreeTreeReplData *[assoc / 2];

  size_t main_depth = (int)round(log2(assoc));
  hot_depth = main_depth - 1;
  cold_depth = hot_depth - 1;
  prob_depth = hot_depth - 1;
}

ThreeTree::~ThreeTree() {
  // delete cold_repl_arr;
  // delete prob_repl_arr;
  // delete hot_repl_arr;
}

void ThreeTree::invalidate(
    const std::shared_ptr<ReplacementData> &replacement_data) {
  std::shared_ptr<ThreeTreeReplData> repl_data =
      std::static_pointer_cast<ThreeTreeReplData>(replacement_data);
  size_t trace_ind = repl_data->tree_index;
  std::vector<bool> &tree = *repl_data->tree;
  while (trace_ind > 0) {
    size_t parent_ind = (trace_ind - 1) / 2;
    if (trace_ind % 2 == 0) {
      // Right child
      tree[parent_ind] = true;
    } else {
      // Left child
      tree[parent_ind] = false;
    }
    trace_ind = parent_ind;
  }
}

void ThreeTree::touch(
    const std::shared_ptr<ReplacementData> &replacement_data) const {
  std::shared_ptr<ThreeTreeReplData> repl_data =
      std::static_pointer_cast<ThreeTreeReplData>(replacement_data);

  std::vector<bool> *subtree = repl_data->tree;
  size_t tree_index = repl_data->tree_index;

  bool should_swap = false;
  ThreeTreeReplData **cur_repl_arr = nullptr;
  ThreeTreeReplData **next_repl_arr = nullptr;
  std::vector<bool> *next_tree = hot_tree;
  size_t next_tree_size;
  size_t next_tree_depth;
  bool chose_cold = false, chose_prob = false;
  if (subtree == cold_tree) {
    next_tree = prob_tree;
    next_tree_depth = prob_depth;
    next_tree_size = assoc / 4;
    cur_repl_arr = cold_repl_arr;
    next_repl_arr = prob_repl_arr;
    should_swap = true;
    chose_cold = true;
  } else if (subtree == prob_tree) {
    next_tree = hot_tree;
    next_tree_depth = hot_depth;
    next_tree_size = assoc / 2;
    cur_repl_arr = prob_repl_arr;
    next_repl_arr = hot_repl_arr;
    should_swap = true;
    chose_prob = true;
  } else {
    should_swap = false;
  }

  size_t touch_ind;
  if (should_swap) {
    // Find LRU of next tree
    size_t trace_ind = 0;
    for (int i = 0; i < next_tree_depth; ++i) {
      if (next_tree->at(trace_ind)) {
        // Trace right
        trace_ind = trace_ind * 2 + 2;
      } else {
        // Trace left
        trace_ind = trace_ind * 2 + 1;
      }
    }

    touch_ind = trace_ind;

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
      next_tree->at(parent_ind) = false;
    } else {
      // Left child
      next_tree->at(parent_ind) = true;
    }
    touch_ind = parent_ind;
  }
}

void ThreeTree::reset(
    const std::shared_ptr<ReplacementData> &replacement_data) const {
  touch(replacement_data);
}

ReplaceableEntry *
ThreeTree::getVictim(const ReplacementCandidates &candidates) const {

  std::vector<bool> *evict_tree;
  size_t tree_choice = rand() % 8;
  size_t trace_ind = 0;
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
    if (evict_tree->at(trace_ind)) {
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
  return candidates.at(0);
}

// std::shared_ptr<ReplacementData> ThreeTree::instantiateEntry() {
//   ThreeTreeReplData *repl = new ThreeTreeReplData(obj_count);
//   if (obj_count < (assoc / 4)) {
//     repl->tree_index = obj_count;
//     repl->tree = cold_tree;
//     this->cold_repl_arr[obj_count] = repl;
//   } else if (obj_count < (assoc / 2)) {
//     repl->tree_index = obj_count - (assoc / 4);
//     repl->tree = prob_tree;
//     this->prob_repl_arr[obj_count - 4] = repl;
//   } else {
//     repl->tree_index = obj_count - (assoc / 2);
//     repl->tree = hot_tree;
//     this->hot_repl_arr[obj_count - 8] = repl;
//   }
//   this->obj_count++;
//   return std::shared_ptr<ReplacementData>(repl);
// }

std::shared_ptr<ReplacementData> ThreeTree::instantiateEntry() {
  size_t cache_ind = this->obj_count % assoc;
  if (cache_ind == 0) {
    // New cache set, make a new set of structures for the next assoc objects
    this->cold_tree = std::make_shared<TTTree>(assoc / 4, false);
    this->prob_tree = std::make_shared<TTTree>(assoc / 4, false);
    this->hot_tree = std::make_shared<TTTree>(assoc / 2, false);
    this->cold_repl_arr = std::make_shared<std::vector<ThreeTreeReplData *>>();
    this->prob_repl_arr = std::make_shared<std::vector<ThreeTreeReplData *>>();
    this->hot_repl_arr = std::make_shared<std::vector<ThreeTreeReplData *>>();
  }

  ThreeTreeReplData *repl = new ThreeTreeReplData(
      cache_ind, this->cold_tree, this->prob_tree, this->hot_tree);

  if (cache_ind < assoc / 4) {
    // Cold queue
    repl->tree_index = cache_ind;
    this->cold_repl_arr->push_back(repl);
  } else if (cache_ind < assoc / 2) {
    // Probation queue
    repl->tree_index = cache_ind - (assoc / 4);
    this->prob_repl_arr->push_back(repl);
  } else {
    // Hot queue
    repl->tree_index = obj_count - (assoc / 2);
    this->hot_repl_arr->push_back(repl);
  }

  this->obj_count++;

  return std::shared_ptr<ReplacementData>(repl);
}

} // namespace replacement_policy
} // namespace gem5
