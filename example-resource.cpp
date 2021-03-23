#include <cmath>
#include <cstdio>
#include <queue>

#include "simcpp.h"

double expovariate(double lambda) {
  double u = rand() / (RAND_MAX + 1.0);
  return -log(1 - u) / lambda;
}

class Resource {
public:
  Resource(simcpp::SimulationPtr sim, int capacity)
      : sim(sim), capacity(capacity) {}

  simcpp::EventPtr request() {
    auto request = sim.lock()->event();
    request_queue.push(request);

    trigger_requests();

    return request;
  }

  void release() {
    ++capacity;

    trigger_requests();
  }

  int get_queue_length() { return request_queue.size(); }

private:
  std::queue<simcpp::EventPtr> request_queue = {};
  simcpp::SimulationWeakPtr sim;
  int capacity;

  void trigger_requests() {
    while (capacity > 0 && request_queue.size() > 0) {
      auto request = request_queue.front();
      request_queue.pop();

      if (!request->is_pending()) {
        continue;
      }

      --capacity;
      request->trigger();
    }
  }
};

using ResourcePtr = std::shared_ptr<Resource>;

class Customer : public simcpp::Process {
public:
  Customer(simcpp::SimulationPtr sim, double mean_time_in_bank,
           double max_wait_time, ResourcePtr counters, int id)
      : Process(sim), mean_time_in_bank(mean_time_in_bank),
        max_wait_time(max_wait_time), counters(counters), id(id) {}

  bool Run() override {
    auto sim = this->sim.lock();

    PT_BEGIN();

    printf("[%5.2f] Customer %d arrives\n", sim->get_now(), id);

    request = counters->request();
    PROC_WAIT_FOR(sim->any_of({request, sim->timeout(max_wait_time)}));

    if (!request->is_triggered()) {
      request->abort();
      printf("[%5.2f] Customer %d leaves unhappy\n", sim->get_now(), id);
      PT_EXIT();
    }

    printf("[%5.2f] Customer %d gets to the counter\n", sim->get_now(), id);

    PROC_WAIT_FOR(sim->timeout(expovariate(1 / mean_time_in_bank)));

    printf("[%5.2f] Customer %d leaves\n", sim->get_now(), id);
    counters->release();

    PT_END();
  }

private:
  simcpp::EventPtr request = nullptr;
  double mean_time_in_bank;
  double max_wait_time;
  ResourcePtr counters;
  int id;
};

class CustomerSource : public simcpp::Process {
public:
  CustomerSource(simcpp::SimulationPtr sim, int n_customers,
                 double mean_arrival_interval, double mean_time_in_bank,
                 double max_wait_time, ResourcePtr counters)
      : Process(sim), n_customers(n_customers),
        mean_arrival_interval(mean_arrival_interval),
        mean_time_in_bank(mean_time_in_bank), max_wait_time(max_wait_time),
        counters(counters) {}

  bool Run() override {
    auto sim = this->sim.lock();

    PT_BEGIN();
    while (next_id <= n_customers) {
      sim->start_process<Customer>(mean_time_in_bank, max_wait_time, counters,
                                   next_id);
      ++next_id;
      PROC_WAIT_FOR(sim->timeout(expovariate(1 / mean_arrival_interval)));
    }
    PT_END();
  }

private:
  int next_id = 1;
  int n_customers;
  double mean_arrival_interval;
  double mean_time_in_bank;
  double max_wait_time;
  ResourcePtr counters;
};

int main() {
  int n_customers = 10;
  double mean_arrival_interval = 10.0;
  double mean_time_in_bank = 12.0;
  double max_wait_time = 16.0;
  int n_counters = 1;

  srand(0);

  auto sim = simcpp::Simulation::create();
  auto counters = std::make_shared<Resource>(sim, n_counters);
  auto customer_source = sim->start_process<CustomerSource>(
      n_customers, mean_arrival_interval, mean_time_in_bank, max_wait_time,
      counters);

  sim->run();

  return 0;
}
