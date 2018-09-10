/**
 * @brief Helper for asynchronous communication
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Helper class to keep track of sent events that are waiting for a response
 */

#ifndef SUSHI_ASYNCHRONOUS_RECEIVER_H
#define SUSHI_ASYNCHRONOUS_RECEIVER_H

#include <vector>
#include <chrono>

#include "library/id_generator.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace receiver {


class AsynchronousEventReceiver
{
public:
    AsynchronousEventReceiver(RtEventFifo* queue) : _queue{queue} {}

    /**
     * @brief Blocks the current thread while waiting for a response to a given event
     * @param id EventId of the event the thread is waiting for
     * @param timeout Maximum wait time
     * @return true if the event was received in time and handled properly, false otherwise
     */
    bool wait_for_response(EventId id, std::chrono::milliseconds timeout);

private:
    struct Node
    {
        EventId id;
        bool    status;
    };
    std::vector<Node> _receive_list;
    RtEventFifo* _queue;
};





} // end namespace receiver
} // end namespace sushi

#endif //SUSHI_ASYNCHRONOUS_RECEIVER_H
