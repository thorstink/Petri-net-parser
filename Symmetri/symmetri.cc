#include "symmetri.h"
#include "actions.h"
#include "model.h"
#include "pnml_parser.h"
#include "types.h"
#include <functional>
#include "ws_interface.hpp"
#include <fstream>

namespace symmetri {
using namespace model;
using namespace actions;
using namespace moodycamel;
using namespace seasocks;

constexpr auto noop = [](const model::Model &m) { return m; };

std::function<void()> start(const std::string &pnml_path,
                            const model::TransitionActionMap &store) {

  return [=]() {
    const auto &[Dm, Dp, M0] = constructTransitionMutationMatrices(pnml_path);

    BlockingConcurrentQueue<Reducer> reducers(256);
    BlockingConcurrentQueue<types::Transition> actions(1024);

    seasocks::Server server(std::make_shared<seasocks::PrintfLogger>(
        seasocks::Logger::Level::Error));
    auto output = std::make_shared<Output>();

    server.addWebSocketHandler("/transition_data", output);
    server.startListening(2222);
    server.setStaticPath("web");
    std::cout << "interface online at http://localhost:2222/" << std::endl;

    auto tp = executeTransition(store, reducers, actions);
    auto m = Model(model::clock_t::now(), M0, Dm, Dp);

    // auto start
    reducers.enqueue(noop);

    std::ofstream output_file;


    while (true) {
      Reducer f = noop;
      reducers.wait_dequeue(f);
      try {
        // run the reducer
        auto [r, transitions] = run(f(m));
        m = std::move(r);
        // dispatch the transitions
        for (auto transition : transitions)
          actions.enqueue(transition);

        m.data->timestamp = model::clock_t::now();
      } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
      }

      //
      auto data = (*m.data);
      m.data->log.clear();
      std::stringstream log_data;
      for (const auto &[a, b, c, d] : data.log) {
        log_data << a << ',' << b << ',' << c << ',' << d << '\n';
      }
      output_file.open("example2.csv",std::ios_base::app); 
      output_file << log_data.str();
      output_file.close();
      // output->send(log_data.str());
      // server.poll(1);
    };
  };
}
} // namespace symmetri
