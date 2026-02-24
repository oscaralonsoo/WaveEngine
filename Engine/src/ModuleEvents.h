#pragma once
#include "Module.h"
#include "Event.h"

#include <map>
#include <memory>
#include <vector>
#include <queue>


class EventListener;

class ModuleEvents : public Module
{
public:
    ModuleEvents();
    ~ModuleEvents();
    
    bool Awake();

    bool CleanUp();

    void Subscribe(Event::Type eventType, EventListener* listener);
    void Unsubscribe(Event::Type eventType, EventListener* listener);
    void UnsubscribeAll(EventListener* listener);

    void PublishImmediate(const Event& event);
    void Publish(std::shared_ptr<Event> event);

    void ProcessEvents();

    void ClearQueue();

    int GetListenerCount(Event::Type eventType) const;
    int GetQueuedEventCount() const { return eventQueue.size(); }

public:
    std::map<Event::Type, std::vector<EventListener*>> listeners;
    std::queue<std::shared_ptr<Event>> eventQueue;

    bool processingEvents;


};