//
// Created by artypoole on 06/07/24.
//

#include "EventQueue.h"
#include "ports.h"
#include "key_maps.h"
#include "logging.h"


static EventQueue* instance{nullptr};


/* US Keyboard Layout. This is a scancode table
*  used to layout a standard US keyboard. I have left some
*  comments in to give you an idea of what key is what, even
*  though I set it's array index to 0. */


/* Handles the keyboard interrupt */
void keyboardHandler()
{
    auto& queue = EventQueue::getInstance();

    /* Read from the keyboard's data buffer */


    if (const u32 scancode = inb(KEYB_DATA); scancode & 0x80) // key down event
    {
        const auto ku = event_t{KEY_UP, event_data_t{scancode-0x80, 0}};
        queue.addEvent(ku);
        /* You can use this one to see if the user released the
        *  shift, alt, or control keys... */
    }
    else // key release event
    {
        const auto kd = event_t{KEY_DOWN, event_data_t{scancode, 0}};
        queue.addEvent(kd);
    }
}

EventQueue::EventQueue()
{

    LOG("Initialising EventQueue");
    instance = this;
    _unread_counter = 0;
    _write_index = 0;
    _read_index = 0;

    for (auto & i : _event_queue)
    {
        i = event_t{NULL_EVENT, event_data_t{0, 0}};
    }
    LOG("EventQueue initialised");
}

EventQueue::~EventQueue()
{
    instance = nullptr;
    _write_index = 0;
}

EventQueue& EventQueue::getInstance()
{
    return *instance;
}

void EventQueue::addEvent(const event_t& event)
{
    _event_queue[_write_index] = event;
    _write_index = (_write_index+1)%max_len;
    _unread_counter++;

}



bool EventQueue::pendingEvents() const
{
    return _unread_counter > 0;
}


event_t EventQueue::getEvent()
{
    if (_unread_counter == 0)
    {

        WRITE("Tried to get read event ahead of event queue. Returning NONE event");
        return event_t{NULL_EVENT, event_data_t{0, 0}};
    }

    const auto event_out = _event_queue[_read_index];
    _unread_counter--;
    _read_index = (_read_index+1)%max_len;
    return event_out;
}