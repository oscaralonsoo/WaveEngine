#pragma once
#include <string>
#include <glm/glm.hpp>
#include "SDL3/SDL.h"

class GameObject;
class Transform;
struct Ray;

struct Event
{
    enum class Type
    {
        //WINDOW
        WindowResize,
        WindowClose,

        //INPUT
        EventSDL,
        FileDropped,

        //SCENE
        SceneLoaded,
        SceneSaved,
        SceneCleared,

        //CAMERAS
        ChangeActiveCamera,

        //GAMEOBJECT
        GameObjectCreated,
        GameObjectDestroyed,
        GameObjectEnabled,
        GameObjectDisabled,
        GameObjectSelected,
        GameObjectDeselected,
        StaticChanged,

        //TRANSFORM
        StaticTransformChanged,

        //UTILS
        CastRay,

        //GAME
        Play,
        Pause,
        Unpause,
        Stop,
        TimeScaleChanged,

        //ASSETS
        AssetsChanged,
        AssetMoved,
        AssetDeleted,

        //RESOURCES
        ResourceDestroyed,

        //RENDER
        PreRender,
        PostRender,

        //OTHERS
        Custom,
        Invalid
    };

    Type type;

    struct UnsignedIntData {
        unsigned int unsignedInt;
    };

    struct Point2dData {
        int x;
        int y;
    };

    struct StringData {
        char string[260];
    };

    struct TwoStringData {
        char string1[260];
        char string2[260];
    };

    struct GameObjectData {
        GameObject* gameObject;
    };

    struct SDLEvent
    {
        SDL_Event* event;
    };

    struct RayData
    {
        Ray* ray;
    };
    struct Bool
    {
        bool boolean;
    };

    union Data
    {
        UnsignedIntData unsignedInt;
        Bool boolean;
        Point2dData point;
        StringData string;
        TwoStringData strings;
        GameObjectData gameObject;
        RayData ray;
        SDLEvent event;
    } data;


    Event(Type t) : type(t) {}

    Event(Type t, unsigned int ui) : type(t) {
        data.unsignedInt.unsignedInt = ui;
    }
    Event(Type t,bool boolean) : type(t) {
        data.boolean.boolean = boolean;
    }

    Event(Type t, int w, int h) : type(t) {
        data.point.x = w;
        data.point.y = h;
    }

    Event(Type t, const char* string) : type(t) {
        strncpy_s(data.string.string, string, 260);
    }

    Event(Type t, const char* string1, const char* string2) : type(t) {
        strncpy_s(data.strings.string1, string1, 260);
        strncpy_s(data.strings.string2, string2, 260);
    }

    Event(Type t, GameObject* gameObject) : type(t) {
        data.gameObject.gameObject = gameObject;
    }

    Event(Type t, Ray* ray) : type(t) {
        data.ray.ray = ray;
    }

    Event(Type t, SDL_Event* event) : type(t) {
        data.event.event = event;
    }

    Event() : type(Type::Invalid) {}
};

