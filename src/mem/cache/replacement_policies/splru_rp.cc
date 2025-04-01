#include "mem/cache/replacement_policies/splru_rp.hh"
#include "base/logging.hh"
#include "params/SplruRP.hh"

namespace gem5 {

namespace replacement_policy {

Splru::SplruReplData::SplruReplData(size_t ind) : leaf_ind(ind) {}

Splru::Splru(const Params &p)
    : Base(p), cold_repl_type(p.cold_repl), hot_repl_type(p.hot_repl),
      probation_type(p.probation_type) {
  fatal_if(p.assoc < 4, "Associativity (assoc) cannot be less than 4");
  tree = new SplruTree(p.assoc);
  count = 0;
  fatal_if(p.cold_repl < 0 || p.cold_repl > 2,
           "Cold Queue replacement flag invalid");
  fatal_if(p.hot_repl < 0 || p.hot_repl > 2,
           "Hot Queue replacement flag invalid");
  fatal_if(p.probation_type < 0 || p.probation_type > 2,
           "Probation choice flag invalid");
}

void Splru::invalidate(const std::shared_ptr<ReplacementData> &repl_data) {
  std::shared_ptr<SplruReplData> splru_repl =
      std::static_pointer_cast<SplruReplData>(repl_data);
  SplruTree *tree = this->tree;
  size_t leaf_ind = splru_repl->leaf_ind;

  size_t cold_ind = leaf_ind;
  size_t cold_assoc = tree->assoc / 4;
  if (leaf_ind > cold_assoc) {
    // Find an element from the cold queue to swap in
    // based on read of the cold queue replacement policy
    size_t cold_swap_ind = tree->get_safe(tree->cold, this->cold_repl_type);
    tree->swap_leaves(leaf_ind, cold_swap_ind);
    cold_ind = cold_swap_ind;
  }

  // cold_ind definitely points to something in the cold queue
  // edit tree according to cold queue eviction policy

  // Invalidation logic: the opposite of touching
  if (this->cold_repl_type == 1) {
    // LRU
    SplruNode *trace = tree->leaf_nodes[cold_ind];
    while (trace->parent != NULL) {
      bool is_left_child = trace->parent->left == trace;
      if (is_left_child) {
        trace->parent->direction = false;
      } else {
        trace->parent->direction = true;
      }
      trace = trace->parent;
    }
  } else if (this->cold_repl_type == 2) {
    // FIFO
    std::stack<SplruNode *> rec_stack;
    rec_stack.push(tree->cold);
    while (!rec_stack.empty()) {
      SplruNode *cur = rec_stack.top();
      rec_stack.pop();
      cur->direction = false;
      if (cur->left != nullptr && cur->right != nullptr) {
        rec_stack.push(cur->left);
        rec_stack.push(cur->right);
      }
    }

    SplruNode *trace = tree->leaf_nodes[cold_ind];
    while (trace->parent != NULL) {
      bool is_left_child = trace->parent->left == trace;
      if (is_left_child) {
        trace->parent->direction = false;
      } else {
        trace->parent->direction = true;
      }
      trace = trace->parent;
    }
  }
}

void Splru::touch(const std::shared_ptr<ReplacementData> &repl_data) const {
  std::shared_ptr<SplruReplData> splru_repl =
      std::static_pointer_cast<SplruReplData>(repl_data);
  SplruTree *tree = this->tree;
  size_t leaf_ind = splru_repl->leaf_ind;
  size_t cold_assoc = tree->assoc / 4;
  size_t hot_ind = leaf_ind;

  // If touching something in the cold queue,
  // select something for eviction from the hot queue
  // and swap it into the cold queue
  if (leaf_ind < cold_assoc) {
    size_t evict_ind = tree->get_victim(tree->hot, this->hot_repl_type);
    tree->swap_leaves(evict_ind, leaf_ind);
    hot_ind = evict_ind;
  }

  // Touch the element in the hot queue
  // If the obj was in the cold queue, it counts as first touch in hot queue
  bool is_first_placement = hot_ind != leaf_ind;
  tree->touch(hot_ind, this->hot_repl_type, is_first_placement);
}

void Splru::reset(const std::shared_ptr<ReplacementData> &repl_data) const {
  touch(repl_data);
}

ReplaceableEntry *
Splru::getVictim(const ReplacementCandidates &candidates) const {
  // Should seek from the cold queue
  // or, if the probation flag allows it, occasionally from the probation area
  SplruNode *trace_node;
  if (probation_type == 0) {
    // Never
    trace_node = this->tree->cold;
  } else if (probation_type == 1) {
    // Half random
    if (rand() % 2 == 0) {
      trace_node = this->tree->cold;
    } else {
      trace_node = this->tree->probation;
    }
  } else if (probation_type == 2) {
    // Quarter random
    if (rand() % 4 == 0) {
      trace_node = this->tree->cold;
    } else {
      trace_node = this->tree->probation;
    }
  }

  int repl_type;
  if (trace_node == this->tree->cold) {
    repl_type = this->cold_repl_type;
  } else {
    repl_type = this->hot_repl_type;
  }

  size_t evict_ind = this->tree->get_victim(trace_node, repl_type);
  return candidates.at(evict_ind);
}

std::shared_ptr<ReplacementData> Splru::instantiateEntry() {
  fatal_if(this->count >= this->tree->assoc,
           "How did count get bigger than assoc?");

  SplruReplData *repl = new SplruReplData(count);
  this->tree->repl_data_arr[count] = repl;
  this->count++;
  return std::shared_ptr<ReplacementData>(repl);
}

} // namespace replacement_policy
} // namespace gem5
