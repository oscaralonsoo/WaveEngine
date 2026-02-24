#pragma once
#include "ModuleEvents.h"
#include "EventListener.h"
#include "Log.h"

ModuleEvents::ModuleEvents() : Module()
{
    name = "ModuleEvents";
}

ModuleEvents::~ModuleEvents()
{

}

bool ModuleEvents::Awake()
{
    processingEvents = false;
    return true;
}

bool ModuleEvents::CleanUp()
{
    ClearQueue();
    return true;
}

void ModuleEvents::Subscribe(Event::Type eventType, EventListener* listener)
{
	if (!listener) return;

    if (std::find(listeners[eventType].begin(), listeners[eventType].end(), listener) == listeners[eventType].end())
    {
        listeners[eventType].push_back(listener);
    }
}

void ModuleEvents::Unsubscribe(Event::Type eventType, EventListener* listener)
{
    if (!listener) return;

    listeners[eventType].erase(
        std::remove(listeners[eventType].begin(), listeners[eventType].end(), listener),
        listeners[eventType].end()
    );
}

void ModuleEvents::UnsubscribeAll(EventListener* listener)
{
    if (!listener) return;

    for (auto& pair : listeners)
    {
        auto& listenerList = pair.second;
        listenerList.erase(
            std::remove(listenerList.begin(), listenerList.end(), listener),
            listenerList.end()
        );
    }
}

void ModuleEvents::PublishImmediate(const Event& event)
{
    if (listeners.find(event.type) == listeners.end())
    {
        return;
    }

    std::vector<EventListener*> listenersCopy = listeners[event.type];

    for (EventListener* listener : listenersCopy)
    {
        if (listener)
        {
            listener->OnEvent(event);
        }
    }
}

void ModuleEvents::Publish(std::shared_ptr<Event> event)
{
    if (!event) return;

    eventQueue.push(event);
}

void ModuleEvents::ProcessEvents()
{
    if (processingEvents)
    {
        LOG_DEBUG("Already processing events, skipping nested call.");
        return;
    }

    processingEvents = true;

    while (!eventQueue.empty())
    {
        std::shared_ptr<Event> event = eventQueue.front();
        eventQueue.pop();

        if (event)
        {
            PublishImmediate(*event);
        }
    }

    processingEvents = false;
}

void ModuleEvents::ClearQueue()
{
    while (!eventQueue.empty())
    {
        eventQueue.pop();
    }
}

int ModuleEvents::GetListenerCount(Event::Type eventType) const
{
    auto it = listeners.find(eventType);
    if (it != listeners.end())
    {
        return it->second.size();
    }
    return 0;
}