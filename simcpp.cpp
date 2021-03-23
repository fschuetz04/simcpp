// Copyright © 2021 Bjørnar Steinnes Luteberget, Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#include "simcpp.h"

namespace simcpp {

/* Simulation */

SimulationPtr Simulation::create() { return std::make_shared<Simulation>(); }

void Simulation::run_process(ProcessPtr process, simtime delay /* = 0.0 */) {
  auto event = this->event();
  event->add_handler(process);
  event->trigger(delay);
}

EventPtr Simulation::timeout(simtime delay) {
  auto event = this->event();
  event->trigger(delay);
  return event;
}

EventPtr Simulation::any_of(std::initializer_list<EventPtr> events) {
  auto any_of_event = event();

  for (auto &event : events) {
    if (event->is_triggered()) {
      any_of_event->trigger();
      return any_of_event;
    }
  }

  auto handler = [any_of_event](simcpp::EventPtr) { any_of_event->trigger(); };

  for (auto &event : events) {
    event->add_handler(handler);
  }

  return any_of_event;
}

EventPtr Simulation::all_of(std::initializer_list<EventPtr> events) {
  auto all_of_event = event();
  int n = events.size();

  for (auto &event : events) {
    if (event->is_triggered()) {
      --n;
    }
  }

  if (n == 0) {
    all_of_event->trigger();
    return all_of_event;
  }

  auto handler = [all_of_event, &n](simcpp::EventPtr) {
    --n;
    if (n == 0) {
      all_of_event->trigger();
    }
  };

  for (auto &event : events) {
    event->add_handler(handler);
  }

  return all_of_event;
}

void Simulation::schedule(EventPtr event, simtime delay /* = 0.0 */) {
  queued_events.emplace(now + delay, next_id, event);
  ++next_id;
}

bool Simulation::step() {
  if (queued_events.empty()) {
    return false;
  }

  auto queued_event = queued_events.top();
  queued_events.pop();
  now = queued_event.time;
  auto event = queued_event.event;
  event->process();
  return true;
}

void Simulation::advance_by(simtime duration) {
  simtime target = now + duration;
  while (has_next() && peek_next_time() <= target) {
    step();
  }
  now = target;
}

bool Simulation::advance_to(EventPtr event) {
  while (event->is_pending() && has_next()) {
    step();
  }

  return event->is_triggered();
}

void Simulation::run() {
  while (step()) {
  }
}

simtime Simulation::get_now() { return now; }

bool Simulation::has_next() { return !queued_events.empty(); }

simtime Simulation::peek_next_time() { return queued_events.top().time; }

/* Simulation::QueuedEvent */

Simulation::QueuedEvent::QueuedEvent(simtime time, size_t id, EventPtr event)
    : time(time), id(id), event(event) {}

bool Simulation::QueuedEvent::operator<(const QueuedEvent &other) const {
  if (time != other.time) {
    return time > other.time;
  }

  return id > other.id;
}

/* Event */

Event::Event(SimulationPtr sim) : sim(sim) {}

bool Event::add_handler(ProcessPtr process) {
  // Handler takes an additional EventPtr arg, but this is ignored by the
  // bound function.
  return add_handler(std::bind(&Process::resume, process));
}

bool Event::add_handler(Handler handler) {
  if (is_triggered()) {
    return false;
  }

  if (is_pending()) {
    handlers.push_back(handler);
  }

  return true;
}

bool Event::trigger(simtime delay /* = 0.0 */) {
  if (!is_pending()) {
    return false;
  }

  auto sim = this->sim.lock();
  sim->schedule(shared_from_this(), delay);

  if (delay == 0.0) {
    state = State::Triggered;
  }

  return true;
}

bool Event::abort() {
  if (!is_pending()) {
    return false;
  }

  state = State::Aborted;
  handlers.clear();

  Aborted();

  return true;
}

void Event::process() {
  if (is_aborted() || is_processed()) {
    return;
  }

  state = State::Processed;

  for (auto &handler : handlers) {
    handler(shared_from_this());
  }

  handlers.clear();
}

bool Event::is_pending() { return state == State::Pending; }

bool Event::is_triggered() {
  return state == State::Triggered || state == State::Processed;
}

bool Event::is_processed() { return state == State::Processed; }

bool Event::is_aborted() { return state == State::Aborted; }

Event::State Event::get_state() { return state; }

void Event::Aborted() {}

/* Process */

Process::Process(SimulationPtr sim) : Event(sim), Protothread() {}

void Process::resume() {
  // Is the process already finished?
  if (!is_pending()) {
    return;
  }

  bool still_running = Run();

  // Did the process finish now?
  if (!still_running) {
    // Process finished
    trigger();
  }
}

ProcessPtr Process::shared_from_this() {
  return std::static_pointer_cast<Process>(Event::shared_from_this());
}

} // namespace simcpp
