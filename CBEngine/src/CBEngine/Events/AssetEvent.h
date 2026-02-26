#pragma once

#include "Event.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Asset/Asset.h"

#include <filesystem>

namespace CB
{
	// ============================================================
	// Asset Events
	// ============================================================

	class AssetImportedEvent : public Event
	{
	public:
		AssetImportedEvent(UUID uuid,AssetType type,const std::filesystem::path& path)
			: m_UUID(uuid), m_AssetType(type), m_Path(path)
		{
		}

		UUID GetAssetUUID() const { return m_UUID; }
		AssetType GetAssetType() const { return m_AssetType; }
		const std::filesystem::path& GetPath() const { return m_Path; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetImportedEvent: " << AssetTypeToString(m_AssetType) << " [" << m_UUID << "] " << m_Path.string();
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetImported)
		EVENT_CLASS_CATEGORY(EventCategoryAsset)
	private:
		UUID m_UUID;
		AssetType m_AssetType;
		std::filesystem::path m_Path;
	};

	class AssetDeletedEvent : public Event
	{
	public:
		AssetDeletedEvent(UUID uuid,AssetType type,const std::filesystem::path& path)
			: m_UUID(uuid), m_AssetType(type), m_Path(path)
		{
		}

		UUID GetAssetUUID() const { return m_UUID; }
		AssetType GetAssetType() const { return m_AssetType; }
		const std::filesystem::path& GetPath() const { return m_Path; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetDeletedEvent: " << AssetTypeToString(m_AssetType) << " [" << m_UUID << "] " << m_Path.string();
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetDeleted)
		EVENT_CLASS_CATEGORY(EventCategoryAsset)
	private:
		UUID m_UUID;
		AssetType m_AssetType;
		std::filesystem::path m_Path;
	};

	class AssetRenamedEvent : public Event
	{
	public:
		AssetRenamedEvent(UUID uuid,const std::filesystem::path& oldPath,const std::filesystem::path& newPath)
			: m_UUID(uuid), m_OldPath(oldPath), m_NewPath(newPath)
		{
		}

		UUID GetAssetUUID() const { return m_UUID; }
		const std::filesystem::path& GetOldPath() const { return m_OldPath; }
		const std::filesystem::path& GetNewPath() const { return m_NewPath; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetRenamedEvent: [" << m_UUID << "] " << m_OldPath.string() << " -> " << m_NewPath.string();
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetRenamed)
		EVENT_CLASS_CATEGORY(EventCategoryAsset)
	private:
		UUID m_UUID;
		std::filesystem::path m_OldPath;
		std::filesystem::path m_NewPath;
	};

	class AssetMovedEvent : public Event
	{
	public:
		AssetMovedEvent(UUID uuid,const std::filesystem::path& oldPath,const std::filesystem::path& newPath)
			: m_UUID(uuid), m_OldPath(oldPath), m_NewPath(newPath)
		{
		}

		UUID GetAssetUUID() const { return m_UUID; }
		const std::filesystem::path& GetOldPath() const { return m_OldPath; }
		const std::filesystem::path& GetNewPath() const { return m_NewPath; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetMovedEvent: [" << m_UUID << "] " << m_OldPath.string() << " -> " << m_NewPath.string();
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetMoved)
		EVENT_CLASS_CATEGORY(EventCategoryAsset)
	private:
		UUID m_UUID;
		std::filesystem::path m_OldPath;
		std::filesystem::path m_NewPath;
	};

	class AssetReloadedEvent : public Event
	{
	public:
		AssetReloadedEvent(UUID uuid,AssetType type)
			: m_UUID(uuid), m_AssetType(type)
		{
		}

		UUID GetAssetUUID() const { return m_UUID; }
		AssetType GetAssetType() const { return m_AssetType; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "AssetReloadedEvent: " << AssetTypeToString(m_AssetType) << " [" << m_UUID << "]";
			return ss.str();
		}

		EVENT_CLASS_TYPE(AssetReloaded)
		EVENT_CLASS_CATEGORY(EventCategoryAsset)
	private:
		UUID m_UUID;
		AssetType m_AssetType;
	};

	// ============================================================
	// Drag & Drop Events
	// ============================================================

	class DragDropBeginEvent : public Event
	{
	public:
		DragDropBeginEvent(const std::string& payloadType,const std::string& sourcePath)
			: m_PayloadType(payloadType), m_SourcePath(sourcePath)
		{
		}

		const std::string& GetPayloadType() const { return m_PayloadType; }
		const std::string& GetSourcePath() const { return m_SourcePath; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "DragDropBeginEvent: " << m_PayloadType << " from " << m_SourcePath;
			return ss.str();
		}

		EVENT_CLASS_TYPE(DragDropBegin)
		EVENT_CLASS_CATEGORY(EventCategoryDragDrop)
	private:
		std::string m_PayloadType;
		std::string m_SourcePath;
	};

	class DragDropEndEvent : public Event
	{
	public:
		DragDropEndEvent(bool accepted)
			: m_Accepted(accepted)
		{
		}

		bool WasAccepted() const { return m_Accepted; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "DragDropEndEvent: " << (m_Accepted ? "accepted" : "cancelled");
			return ss.str();
		}

		EVENT_CLASS_TYPE(DragDropEnd)
		EVENT_CLASS_CATEGORY(EventCategoryDragDrop)
	private:
		bool m_Accepted;
	};

	class DragDropDeliverEvent : public Event
	{
	public:
		DragDropDeliverEvent(const std::string& payloadType,const std::string& sourcePath,
		                     const std::string& targetPath)
			: m_PayloadType(payloadType), m_SourcePath(sourcePath), m_TargetPath(targetPath)
		{
		}

		const std::string& GetPayloadType() const { return m_PayloadType; }
		const std::string& GetSourcePath() const { return m_SourcePath; }
		const std::string& GetTargetPath() const { return m_TargetPath; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "DragDropDeliverEvent: " << m_PayloadType << " " << m_SourcePath << " -> " << m_TargetPath;
			return ss.str();
		}

		EVENT_CLASS_TYPE(DragDropDeliver)
		EVENT_CLASS_CATEGORY(EventCategoryDragDrop)
	private:
		std::string m_PayloadType;
		std::string m_SourcePath;
		std::string m_TargetPath;
	};
}