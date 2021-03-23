#include <gtest/gtest.h>

#include "simcpp.h"

class Awaiter : public simcpp::Process {
public:
  Awaiter(simcpp::SimulationPtr sim, simcpp::EventPtr event)
      : Process(sim), event(event) {}

  bool Run() override {
    PT_BEGIN();
    PROC_WAIT_FOR(event);
    PT_END();
  }

private:
  simcpp::EventPtr event;
};

TEST(SimulationTest, AnyOfEmpty) {
  auto sim = simcpp::Simulation::create();

  auto any_of = sim->any_of({});
  auto awaiter = sim->start_process<Awaiter>(any_of);

  ASSERT_EQ(sim->get_now(), 0);
  sim->advance_to(awaiter);
  ASSERT_EQ(sim->get_now(), 0);
}

TEST(SimulationTest, AnyOfTriggered) {
  auto sim = simcpp::Simulation::create();

  auto event1 = sim->timeout(5);
  auto event2 = sim->event();
  event2->trigger();
  auto any_of = sim->any_of({event1, event2});
  auto awaiter = sim->start_process<Awaiter>(any_of);

  ASSERT_EQ(sim->get_now(), 0);
  sim->advance_to(awaiter);
  ASSERT_EQ(sim->get_now(), 0);
}

TEST(SimulationTest, AnyOfPending) {
  auto sim = simcpp::Simulation::create();

  auto event1 = sim->timeout(5);
  auto event2 = sim->timeout(10);
  auto any_of = sim->any_of({event1, event2});
  auto awaiter = sim->start_process<Awaiter>(any_of);

  ASSERT_EQ(sim->get_now(), 0);
  sim->advance_to(awaiter);
  ASSERT_EQ(sim->get_now(), 5);
}

TEST(SimulationTest, AllOfEmpty) {
  auto sim = simcpp::Simulation::create();

  auto all_of = sim->all_of({});
  auto awaiter = sim->start_process<Awaiter>(all_of);

  ASSERT_EQ(sim->get_now(), 0);
  sim->advance_to(awaiter);
  ASSERT_EQ(sim->get_now(), 0);
}

TEST(SimulationTest, AllOfTriggered) {
  auto sim = simcpp::Simulation::create();

  auto event1 = sim->event();
  event1->trigger();
  auto event2 = sim->event();
  event2->trigger();
  auto all_of = sim->all_of({event1, event2});
  auto awaiter = sim->start_process<Awaiter>(all_of);

  ASSERT_EQ(sim->get_now(), 0);
  sim->advance_to(awaiter);
  ASSERT_EQ(sim->get_now(), 0);
}

TEST(SimulationTest, AllOfPending) {
  auto sim = simcpp::Simulation::create();

  auto event1 = sim->timeout(5);
  auto event2 = sim->timeout(10);
  auto all_of = sim->all_of({event1, event2});
  auto awaiter = sim->start_process<Awaiter>(all_of);

  ASSERT_EQ(sim->get_now(), 0);
  sim->advance_to(awaiter);
  ASSERT_EQ(sim->get_now(), 10);
}
