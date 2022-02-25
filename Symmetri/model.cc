#include "model.h"

namespace symmetri {

Model run_all(Model model) {
  Transitions T;

  for (const auto &[T_i, mut] : model.data->net) {
    const auto &pre = mut.first;
    if (!pre.empty() &&
        std::all_of(std::begin(pre), std::end(pre), [&](const auto &m_p) {
          return model.data->M[m_p] >= pre.count(m_p);
        })) {
      for (auto &m_p : pre) {
        model.data->M[m_p] -= 1;
      }
      model.data->iteration++;
      T.push_back(T_i);
      model.data->active_transitions.insert(T_i);
    }
  }
  model.transitions_->enqueue_bulk(T.begin(), T.size());
  return model;
}

}  // namespace symmetri
