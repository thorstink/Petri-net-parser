#include "model.h"

#include <numeric>

#include "draw_ui.h"
#include "menu_bar.h"
#include "petri.h"
namespace model {

Coordinate operator+(Coordinate& lhs, Coordinate& rhs) {
  return {lhs.x + rhs.x, lhs.y + rhs.y};
}
Coordinate& operator+=(Coordinate& lhs, const Coordinate& rhs) {
  lhs.x += rhs.x;
  lhs.y += rhs.y;
  return lhs;
}

ViewModel::ViewModel(Model m)
    : drawables([&] {
        std::vector<Drawable> drawables;
        drawables.push_back(&draw_menu_bar);
        drawables.push_back(&draw_interface);
        for (auto&& drawable : m.data->drawables) {
          drawables.push_back(drawable);
        }
        return drawables;
      }()),
      show_grid(m.data->show_grid),
      scrolling(m.data->scrolling),
      selected_arc_idxs(m.data->selected_arc_idxs),
      selected_node_idx(m.data->selected_node_idx),
      selected_target_node_idx(m.data->selected_target_node_idx),
      active_file(
          m.data->active_file.value_or(std::filesystem::current_path())),
      t_view(m.data->t_view),
      p_view(m.data->p_view),
      colors(m.data->colors),
      tokens(m.data->tokens),
      net(m.data->net),
      t_positions(m.data->t_positions),
      p_positions(m.data->p_positions),
      t_fireable(std::accumulate(
          t_view.begin(), t_view.end(), std::vector<size_t>{},
          [this](std::vector<size_t>&& t_fireable, size_t t_idx) {
            if (canFire(net.input_n[t_idx], tokens)) {
              t_fireable.push_back(t_idx);
            }
            return std::move(t_fireable);
          })) {}

}  // namespace model
