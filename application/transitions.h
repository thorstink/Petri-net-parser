#pragma once
#include "Symmetri/model.h"

model::OptionalReducer action0();
model::OptionalReducer action1();
model::OptionalReducer action2();
model::OptionalReducer action3();
model::OptionalReducer action4();
model::OptionalReducer action5();
model::OptionalReducer action6();

inline model::TransitionActionMap getStore() {
  return {{"t0", &action0}, {"t1", &action1}, {"t2", &action2},
          {"t3", &action3}, {"t4", &action4}, {"t5", &action5},
          {"t6", &action6}};
};

// const static TransitionActionMap local_store = {
//     {"t18", &action0}, {"t19", &action1}, {"t20", &action2}, {"t21", &action3},
//     {"t22", &action4}, {"t23", &action5}, {"t24", &action6}, {"t25", &action3},
//     {"t26", &action4}, {"t27", &action5}, {"t28", &action6}, {"t29", &action6}};
