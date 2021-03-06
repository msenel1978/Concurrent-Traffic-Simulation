#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
#define WAIT_TIME_MS 1
 
/* MessageQueue implementation similar to the course example */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a-DONE : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    // pass unique lock to condition variable
    _queueCond.wait(uLock, [this] { return !_queue.empty(); });

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a-DONE : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> uLock(_mutex);

    // Add a new message to the queue
    _queue.push_back(std::move(msg));

    // Send a notification
    _queueCond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b-DONE : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    TrafficLightPhase light_phase;

    while (true) {

        light_phase = _messageQueue.receive();

        if (light_phase == TrafficLightPhase::green)
            return;

        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MS));
    }
}


TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b-DONE : Finally, the private method „cycleThroughPhases“ should be started in a thread 
    // when the public method „simulate“ is called. To do this, use the this_thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));

}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a-DONE : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::random_device rd;
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_real_distribution<> distr(4000.0, 6000.0);
    
    /* The cycle duration should be a random value between 4 and 6 seconds */
    double cycle_duration = distr(eng);

    /* Init stop watch */
    auto last_update = std::chrono::system_clock::now();

    /* Variable to measure time since last update */
    long time_since_last_update;

    while (true) {

        /* Time betwen cycles (milliseconds) */
        time_since_last_update =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                last_update).count();

        if (time_since_last_update >= cycle_duration) {
            // Toggle TrafficLight
            _currentPhase = (_currentPhase == TrafficLightPhase::red) ?
                                TrafficLightPhase::green : TrafficLightPhase::red;

            //Send an update method to the message queue using move semantics
            _messageQueue.send(std::move(_currentPhase));

            /* Reset stop watch for next cycle */
            last_update = std::chrono::system_clock::now();

            /* Cycle duration for the next cycle */
            cycle_duration = distr(eng);

        }

        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_MS));
    }
}