#pragma once

#include "Event.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Core/String.h"

namespace CB
{
    // ============================================================
    // Scene Events
    // ============================================================

    class SceneLoadedEvent : public Event
    {
    public:
        SceneLoadedEvent(const String& name, const String& path)
            : m_Name(name), m_Path(path)
        {
        }

        const String& GetSceneName() const { return m_Name; }
        const String& GetScenePath() const { return m_Path; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "SceneLoadedEvent: " << m_Name << " (" << m_Path << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(SceneLoaded)
        EVENT_CLASS_CATEGORY(EventCategoryScene)

    private:
        String m_Name;
        String m_Path;
    };

    class SceneUnloadedEvent : public Event
    {
    public:
        SceneUnloadedEvent(const String& name)
            : m_Name(name)
        {
        }

        const String& GetSceneName() const { return m_Name; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "SceneUnloadedEvent: " << m_Name;
            return ss.str();
        }

        EVENT_CLASS_TYPE(SceneUnloaded)
        EVENT_CLASS_CATEGORY(EventCategoryScene)

    private:
        String m_Name;
    };

    class SceneSavedEvent : public Event
    {
    public:
        SceneSavedEvent(const String& name, const String& path)
            : m_Name(name), m_Path(path)
        {
        }

        const String& GetSceneName() const { return m_Name; }
        const String& GetScenePath() const { return m_Path; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "SceneSavedEvent: " << m_Name << " (" << m_Path << ")";
            return ss.str();
        }

        EVENT_CLASS_TYPE(SceneSaved)
        EVENT_CLASS_CATEGORY(EventCategoryScene)

    private:
        String m_Name;
        String m_Path;
    };

    // ============================================================
    // Entity Events
    // ============================================================

    class EntityCreatedEvent : public Event
    {
    public:
        EntityCreatedEvent(UUID entityUUID, const String& name)
            : m_EntityUUID(entityUUID), m_Name(name)
        {
        }

        UUID GetEntityUUID() const { return m_EntityUUID; }
        const String& GetEntityName() const { return m_Name; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "EntityCreatedEvent: " << m_Name << " [" << m_EntityUUID << "]";
            return ss.str();
        }

        EVENT_CLASS_TYPE(EntityCreated)
        EVENT_CLASS_CATEGORY(EventCategoryEntity)

    private:
        UUID m_EntityUUID;
        String m_Name;
    };

    class EntityDestroyedEvent : public Event
    {
    public:
        EntityDestroyedEvent(UUID entityUUID, const String& name)
            : m_EntityUUID(entityUUID), m_Name(name)
        {
        }

        UUID GetEntityUUID() const { return m_EntityUUID; }
        const String& GetEntityName() const { return m_Name; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "EntityDestroyedEvent: " << m_Name << " [" << m_EntityUUID << "]";
            return ss.str();
        }

        EVENT_CLASS_TYPE(EntityDestroyed)
        EVENT_CLASS_CATEGORY(EventCategoryEntity)

    private:
        UUID m_EntityUUID;
        String m_Name;
    };

    class ComponentAddedEvent : public Event
    {
    public:
        ComponentAddedEvent(UUID entityUUID, const String& componentName)
            : m_EntityUUID(entityUUID), m_ComponentName(componentName)
        {
        }

        UUID GetEntityUUID() const { return m_EntityUUID; }
        const String& GetComponentName() const { return m_ComponentName; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "ComponentAddedEvent: " << m_ComponentName << " on entity [" << m_EntityUUID << "]";
            return ss.str();
        }

        EVENT_CLASS_TYPE(ComponentAdded)
        EVENT_CLASS_CATEGORY(EventCategoryEntity)

    private:
        UUID m_EntityUUID;
        String m_ComponentName;
    };

    class ComponentRemovedEvent : public Event
    {
    public:
        ComponentRemovedEvent(UUID entityUUID, const String& componentName)
            : m_EntityUUID(entityUUID), m_ComponentName(componentName)
        {
        }

        UUID GetEntityUUID() const { return m_EntityUUID; }
        const String& GetComponentName() const { return m_ComponentName; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "ComponentRemovedEvent: " << m_ComponentName << " on entity [" << m_EntityUUID << "]";
            return ss.str();
        }

        EVENT_CLASS_TYPE(ComponentRemoved)
        EVENT_CLASS_CATEGORY(EventCategoryEntity)

    private:
        UUID m_EntityUUID;
        String m_ComponentName;
    };
}
