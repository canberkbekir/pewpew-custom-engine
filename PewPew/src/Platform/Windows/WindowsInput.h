#pragma once
#include "PewPew/Input.h"

namespace PewPew
{
	class WindowsInput : public Input
	{
	protected:
		~WindowsInput() = default;
		virtual bool IsKeyPressedImpl(int keycode) override;

		virtual bool IsMouseButtonPressedImpl(int button) override;
		virtual std::pair<float, float> GetMousePositionImpl() override;
		virtual float GetMouseXImpl() override;
		virtual float GetMouseYImpl() override;
	};
}