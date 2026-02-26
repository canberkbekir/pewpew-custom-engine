#pragma once
#include "String.h"
#include "TimeStep.h"
#include "CBEngine/Events/Event.h"

namespace CB
{
	class Layer
	{
	public:
		Layer(const String& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach()
		{
		}

		virtual void OnDetach()
		{
		}

		virtual void OnUpdate(Timestep ts)
		{
		}

		virtual void OnImGuiRender()
		{
		}

		virtual void OnEvent(Event& event)
		{
		}

		const String& GetName() const { return m_DebugName; }
	protected:
		String m_DebugName;
	};
}