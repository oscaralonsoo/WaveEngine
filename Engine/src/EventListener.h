#pragma once

class Event;

class EventListener
{
public:
    virtual ~EventListener() = default;

    virtual void OnEvent(const Event& event) = 0;
};