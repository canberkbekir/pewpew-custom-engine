#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Components/TransformComponent.h"
#include "../EditorUtils.h"
#include "../ComponentCard.h"

#include <cmath>

namespace CB
{
    class TransformComponentView
    {
    public:
        static void Draw(Entity entity)
        {
            if (!entity.HasComponent<TransformComponent>())
                return;

            bool reset = false;

            bool opened = ComponentCard::Begin("Transform", false, nullptr, &reset);

            if (reset)
            {
                auto& t = entity.GetComponent<TransformComponent>();
                t.Position = Vector3(0.0f);
                t.Rotation = Vector3(0.0f);
                t.Scale = Vector3(1.0f);
            }

            if (opened)
            {
                auto& transform = entity.GetComponent<TransformComponent>();

                if (ImGui::BeginTable("##TransformTable", 2,
                                      ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    // Position
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Position");
                    ImGui::TableSetColumnIndex(1);
                    DrawVec3Control("##Position", transform.Position);

                    // Rotation (store radians, show degrees)
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Rotation");
                    ImGui::TableSetColumnIndex(1);

                    Vector3 rotationDegrees = degrees(transform.Rotation);
                    if (DrawVec3Control("##Rotation", rotationDegrees))
                        transform.Rotation = radians(rotationDegrees);

                    // Scale with lock button
                    static bool scaleLocked = true;
                    static uint64_t s_LastEntityId = 0;
                    static Vector3 s_LastScale(1.0f, 1.0f, 1.0f);

                    const uint64_t entityId = entity.GetUUID();
                    if (s_LastEntityId != entityId)
                    {
                        s_LastEntityId = entityId;
                        s_LastScale = transform.Scale;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Scale");
                    ImGui::TableSetColumnIndex(1);

                    Vector3 currentScale = transform.Scale;

                    const float buttonSize = ImGui::GetFrameHeight();
                    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
                    const float avail = ImGui::GetContentRegionAvail().x;
                    const float vecWidth = ImMax(1.0f, avail - (buttonSize + spacing));

                    ImGui::BeginGroup();
                    ImGui::PushItemWidth(vecWidth);

                    const bool scaleEdited = DrawVec3Control("##Scale", currentScale, 1.0f);

                    ImGui::PopItemWidth();

                    ImGui::SameLine(0.0f, spacing);

                    if (ImGui::Button("##ScaleLock", ImVec2(buttonSize, buttonSize)))
                        scaleLocked = !scaleLocked;

                    ImGui::EndGroup();

                    if (scaleEdited)
                    {
                        if (scaleLocked)
                        {
                            Vector3 delta = currentScale - s_LastScale;

                            float ratio = 0.0f;
                            if (fabs(delta.x) > 0.0001f && fabs(s_LastScale.x) > 0.000001f)
                                ratio = currentScale.x / s_LastScale.x;
                            else if (fabs(delta.y) > 0.0001f && fabs(s_LastScale.y) > 0.000001f)
                                ratio = currentScale.y / s_LastScale.y;
                            else if (fabs(delta.z) > 0.0001f && fabs(s_LastScale.z) > 0.000001f)
                                ratio = currentScale.z / s_LastScale.z;

                            if (ratio != 0.0f)
                                transform.Scale = s_LastScale * ratio;
                        }
                        else
                        {
                            transform.Scale = currentScale;
                        }

                        s_LastScale = transform.Scale;
                    }
                    else
                    {
                        s_LastScale = transform.Scale;
                    }

                    ImGui::EndTable();
                }
            }

            ComponentCard::End();
        }
    };
}
