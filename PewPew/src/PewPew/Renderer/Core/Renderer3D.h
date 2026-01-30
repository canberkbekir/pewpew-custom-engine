#pragma once
#include "PewPew/Renderer/Camera/Camera.h"
#include "PewPew/Renderer/Resources/Shader.h"
#include "PewPew/Renderer/Resources/Material.h"
#include "PewPew/Renderer/Resources/Mesh.h"

namespace PewPew
{
    class Renderer3D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const Camera& camera, const Vector3& cameraPosition);
        static void EndScene();

        // PBR submit with Material
        static void Submit(const Ref<Shader>& shader, const Ref<Material>& material, const Ref<Mesh>& mesh,
                           const Mat4& transform = Mat4(1.0f));

        // Light configuration
        static void SetDirectionalLight(const Vector3& direction, const Vector3& color, float intensity);
        static void SetAmbientLight(const Vector3& color);

    private:
        struct SceneData
        {
            Mat4 ViewProjectionMatrix;
            Vector3 CameraPosition = {0.0f, 0.0f, 0.0f};

            // Directional light
            Vector3 LightDirection = {-0.5f, -1.0f, -0.3f};
            Vector3 LightColor = {1.0f, 1.0f, 1.0f};
            float LightIntensity = 1.0f;
            Vector3 AmbientColor = {0.03f, 0.03f, 0.03f};
        };

        static Scope<SceneData> s_SceneData;
    };
}
