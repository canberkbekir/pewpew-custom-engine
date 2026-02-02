#include <PewPew.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "EditorLayer.h"

// ============================================================================
// PewPew Editor Application
// ============================================================================
class PewPewEditor : public PewPew::Application
{
public:
	PewPewEditor()
	{
		PEW_PROFILE_FUNCTION();
		PushLayer(new PewPew::EditorLayer());
	}

	~PewPewEditor() {}
};

PewPew::Application* PewPew::CreateApplication()
{
	PEW_PROFILE_FUNCTION();
	return new PewPewEditor();
}
