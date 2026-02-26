#include <CBEngine.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "EditorLayer.h"

// ============================================================================
// CBEngine Editor Application
// ============================================================================
class CBEngineEditor : public CB::Application
{
public:
	CBEngineEditor()
	{
		CB_PROFILE_FUNCTION();
		PushLayer(new CB::EditorLayer());
	}

	~CBEngineEditor() override
	{
	}
};

CB::Application* CB::CreateApplication()
{
	CB_PROFILE_FUNCTION();
	return new CBEngineEditor();
}