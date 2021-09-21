#include "model.h"

namespace model {

std::pair<Model, types::Transitions> run(Model model) {
  auto &M = model.data->M;
  const auto &Dm = model.data->Dm;
  types::Transitions T;

  for (const auto &[T_i, dM_i] : Dm) {
    if (((M - dM_i).array() >= 0).all()) {
      M -= dM_i;
      T.push_back(T_i);
    }
  }

  return {model, T};
}

} // namespace model