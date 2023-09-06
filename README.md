# Symmetri

A C++-17 library that takes a Petri net and turns it into a program. This is done by mapping *[transitions](https://en.wikipedia.org/wiki/Petri_net#Petri_net_basics)* to *functions* and calling the functions for which their transition counterpart is *[fireable](https://en.wikipedia.org/wiki/Petri_net#Execution_semantics)*. Petri nets are a graphical language that naturally can model concurrent and distributed systems ([*wikipedia*](https://en.wikipedia.org/wiki/Petri_net#Petri_net_basics)).

## Principle

The graph below represents a Petri net. The transitions `bar` is fireable, and if it would fire it would consume the *token* (<span style="color:#00c0cb;">&#9679;</span>) and produce two new tokens in the output places. These new tokens would enable `foo` and the cycle would continue for ever, while accumelating tokens in the most left place.

```mermaid
    graph LR
    classDef inactive fill:#ffc0;
    classDef active fill:#0050cb;
    classDef fake_tok_place color:#ffc0;
    classDef place color:#00c0cb;

    B-->D[foo]:::inactive;
    Z(("#9679;")):::place-->A
    A[bar]:::active-->B(("#9679;" )):::fake_tok_place;
    A-->C(("#9679;")):::fake_tok_place;
    C-->D;
    D-->Z
    D-->B;
```

This Petri net can be described using Symmetri:

```cpp
using namespace symmetri;
Net net = {{"foo", {{"B", "C"}, {"Z", "B"}}},
                {"bar", {{"Z"}, {"B", "C"}}}};
Store store = {{"foo", &foo}, {"bar", &bar}};
PriorityTable priority = {};
Marking m0 = {{"Z", 1}, {"B", 0}, {"C", 0}};
auto pool = std::make_shared<symmetri::TaskSystem>(4);
symmetri::PetriNet app(net, m0, {}, store, priority,
                          "test_net_without_end", pool);
auto res = app.fire(); // run until done.
```

- `net` is a multiset description of a Petri net
- `store` is a lookup table that links the *symbolic* transitions to actual functions
- `priority` can be used if some transitions are more equal than others ([Wiki on prioritized Petri nets](https://en.wikipedia.org/wiki/Prioritised_Petri_net))
- `m0` is the initial token distribution (also known as _initial marking_)
- `pool` is a simple task based threadpool
- `app` is all the ingredients put together - creating something that can be executed! it outputs a result (`res`) and a log (`eventlog`)

## Build

Clone the repository and make sure you also initialize the submodules:

```bash
git clone https://github.com/thorstink/Symmetri.git
git submodule update --init --recursive
mkdir build
cd build
# Debug build without sanitizers
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_EXAMPLES=ON -DBUILD_TESTING=ON -DASAN_BUILD=OFF -DTSAN_BUILD=OFF ..
# Building and running tests:
make && make test
```

## Run examples

```bash
# assumed you build the examples
# finishes with a deadlock
./build/examples/hello_world/symmetri_hello_world nets/passive_n1.pnml nets/T50startP0.pnml
# finishes by reaching the final marking (e.g. completed). Use keyboard to interact pause/resume/cancel/print log
./build/examples/flight/symmetri_flight nets/PT1.pnml nets/PT2.pnml nets/PT3.pnml
```

## Implementation

Some details on the implementation.

<p align="center" width="100%">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="./docs/img/architecture_dark.svg">
  <source media="(prefers-color-scheme: light)" srcset="./docs/img/architecture_light.svg">
  <img alt="architecture" src="./docs/img/architecture_light.svg">
</picture>
</p>

## State of the library

Still in alpha! Improving as we go. None the less, Symmetri is already being used in industry ;-). Feel free to contact me if you are curious or have questions.
