#include "../EditorLayer.h"

namespace CB
{
	void RequestImportPreviewFromWidget(const std::filesystem::path& path)
	{
		EditorLayer::RequestImportPreview(path);
	}

	void RequestReimportFromWidget(UUID uuid)
	{
		EditorLayer::RequestImportPreviewReimport({}, uuid);
	}
}
