#pragma once

#include <symmetri/parsers.h>
#include <symmetri/symmetri.h>
#include <symmetri/types.h>

#include <cassert>
#include <filesystem>
#include <functional>
#include <optional>
#include <vector>

#include "blockingconcurrentqueue.h"
#include "drawable.h"
#include "graph.hpp"
#include "imgui.h"

using View = std::vector<Drawable>;
using History = std::vector<View>;

inline void commit(History& x) {
  assert(!x.empty());
  x.push_back(x.back());
}
inline void undo(History& x) {
  assert(!x.empty());
  x.pop_back();
}
inline View& current(History& x) {
  assert(!x.empty());
  return x.back();
}

struct ViewModel {};

struct Model {
  std::shared_ptr<Graph> graph;
  std::filesystem::path working_dir;
  std::optional<std::filesystem::path> active_file;
  int menu_height = 0;
  View statics;
};

void draw(Model& m);

using Reducer = std::function<Model(Model)>;

class MVC {
 private:
  inline static moodycamel::BlockingConcurrentQueue<Reducer> reducers{10};

 public:
  inline static Model model;
  inline static void render() { draw(model); };
  inline static void update(const Reducer& f) { model = f(model); }
  inline static void push(Reducer&& f) {
    reducers.enqueue(std::forward<Reducer>(f));
  }
  inline static std::optional<Reducer> dequeue() {
    static Reducer r;
    return reducers.try_dequeue(r) ? std::optional(r) : std::nullopt;
  }
};
