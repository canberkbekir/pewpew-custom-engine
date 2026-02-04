#pragma once
#include "Scene.h"
#include "PewPew/Asset/Asset.h"

namespace PewPew
{
	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& ActiveScene);

		void Serialize(const String& FilePath);
		void Deserialize(const String& FilePath);

		//YAML format
		void SerializeText(const String& FilePath);
		void DeserializeText(const String& FilePath);

	private:
		Ref<Scene> m_Scene;
		
	};

}