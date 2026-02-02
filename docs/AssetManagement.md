# Asset Management System

## Overview

The Asset Management System provides a centralized way to track, load, and manage all game assets (textures, meshes, shaders, materials, scenes). It uses UUIDs to identify assets, enabling references to survive file renames and moves.

---

## Core Concepts

### UUID (Universally Unique Identifier)

Every asset gets a unique 64-bit identifier when first imported. This UUID never changes, even if the file is renamed or moved. Other systems reference assets by UUID, not by file path.

**Why UUIDs?**
- File path changes don't break references
- Multiple assets can have the same filename (in different folders)
- Enables dependency tracking between assets

### .meta Files

Each asset has a companion `.meta` file storing its UUID and metadata.

```
assets/
├── textures/
│   ├── player.png
│   ├── player.png.meta    <-- Contains UUID, type
│   ├── enemy.png
│   └── enemy.png.meta
├── models/
│   ├── character.fbx
│   └── character.fbx.meta
```

When you move `player.png` to another folder, its `.meta` file moves with it. The UUID stays the same, so all references remain valid.

---

## System Architecture

### File Structure

```
PewPew/src/PewPew/
├── Asset/
│   ├── UUID.h              -- 64-bit unique identifier
│   ├── Asset.h             -- Base asset class + AssetType enum
│   ├── AssetMetadata.h     -- Metadata structure
│   ├── AssetRegistry.h/cpp -- UUID <-> path mapping
│   ├── AssetManager.h/cpp  -- Main facade
│   └── AssetHandle.h       -- Lightweight reference
│
├── Selection/
│   ├── Selectable.h        -- Selection item structure
│   └── Selection.h/cpp     -- Global selection singleton
│
└── FileWatcher/
    └── FileWatcher.h       -- File change detection

Platform/Windows/
└── WindowsFileWatcher.cpp  -- Windows ReadDirectoryChangesW implementation
```

### Asset Types

```cpp
enum class AssetType
{
    None = 0,
    Texture2D,
    Mesh,
    Shader,
    Material,
    Scene
};
```

---

## Initialization

The system is automatically initialized in `Application`:

```cpp
// Application.cpp

Application::Application()
{
    // ... renderer init ...
    AssetManager::Init("assets");  // Scans assets folder, loads .meta files
}

void Application::Run()
{
    while (m_Running)
    {
        AssetManager::ProcessReloadQueue();  // Process hot reloads
        // ... rest of loop ...
    }
}

Application::~Application()
{
    AssetManager::Shutdown();
}
```

---

## Hot Reload

The system automatically detects file changes and reloads assets.

### How It Works

```
┌─────────────────┐
│   File System   │  User saves texture in Photoshop
└────────┬────────┘
         │ file modified
         ▼
┌─────────────────┐
│   FileWatcher   │  Background thread detects change
└────────┬────────┘
         │ AssetManager::QueueReload(uuid)
         ▼
┌─────────────────┐
│  Reload Queue   │  Thread-safe queue
└────────┬────────┘
         │ Main thread: ProcessReloadQueue()
         ▼
┌─────────────────┐
│  asset->Reload()│  Re-reads file from disk
└────────┬────────┘
         │ Cascade to dependents
         ▼
┌─────────────────┐
│   Materials     │  Materials using this texture also reload
└─────────────────┘
```

### FileWatcher Setup (EditorLayer)

```cpp
void EditorLayer::OnAttach()
{
    m_FileWatcher = CreateScope<FileWatcher>("assets", [](const FileWatcherEvent& event)
    {
        if (event.Action == FileAction::Modified)
        {
            std::filesystem::path relativePath = std::filesystem::relative(event.FilePath, "assets");
            UUID uuid = AssetManager::GetRegistry().GetUUIDByPath(relativePath);

            if (uuid.IsValid())
            {
                AssetManager::QueueReload(uuid);
            }
        }
    });
    m_FileWatcher->Start();
}

void EditorLayer::OnDetach()
{
    m_FileWatcher->Stop();
}
```

### Reloadable Resources

All core resources support hot reload:

| Resource | Reload Behavior |
|----------|-----------------|
| Texture2D | Deletes old GPU texture, reloads image from disk |
| Shader | Recompiles from source file, keeps old program if compile fails |
| Mesh | Reloads from FBX/OBJ/GLTF file |

---

## Selection System

Unified selection across Content Browser and Scene Hierarchy.

### Selectable Types

```cpp
enum class SelectableType
{
    None,
    Asset,   // Selected from Content Browser
    Entity   // Selected from Scene Hierarchy
};

struct Selectable
{
    SelectableType Type;
    UUID ID;
};
```

### Usage

```cpp
// Select an asset
Selection::Select(Selectable::Asset(assetUUID));

// Select an entity
Selection::Select(Selectable::Entity(entityUUID));

// Multi-select (Ctrl+Click)
Selection::AddToSelection(Selectable::Asset(anotherUUID));

// Check selection
if (Selection::HasSelection())
{
    Selectable primary = Selection::GetPrimarySelection();

    if (primary.Type == SelectableType::Asset)
    {
        // Show asset properties
    }
    else if (primary.Type == SelectableType::Entity)
    {
        // Show entity components
    }
}

// Clear selection
Selection::Clear();
```

### Selection Callbacks

```cpp
Selection::AddSelectionChangedCallback([](const std::vector<Selectable>& selection)
{
    // React to selection changes
    PEW_INFO("Selection changed: {0} items", selection.size());
});
```

---

## Content Browser Integration

### Selection Flow

```
User clicks asset in Content Browser
         │
         ▼
ContentBrowserPanel::SelectAsset(path)
         │
         ├─── Get UUID from AssetRegistry
         │
         ▼
Selection::Select(Selectable::Asset(uuid))
         │
         ▼
Selection notifies callbacks
         │
         ▼
PropertiesPanel checks Selection::GetPrimarySelection()
         │
         ▼
PropertiesPanel::DrawAssetProperties(uuid)
```

### Drag & Drop

Content Browser supports drag & drop with payload type `"CONTENT_BROWSER_ITEM"`:

```cpp
// In receiving panel (e.g., Viewport, Scene Hierarchy)
if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
    {
        const char* pathStr = static_cast<const char*>(payload->Data);
        std::filesystem::path path(pathStr);

        // Create entity with this asset
        // ...
    }
    ImGui::EndDragDropTarget();
}
```

---

## Asset Handle

Lightweight reference (8 bytes) that survives hot reload.

### Why Use Handles?

```cpp
// BAD: Direct pointer breaks on hot reload
Ref<Texture2D> m_Texture;  // Pointer becomes invalid after reload

// GOOD: Handle resolves to new pointer after reload
AssetHandle<Texture2D> m_Texture;  // UUID stays valid, Get() returns new pointer
```

### Usage

```cpp
// Store handle
AssetHandle<Texture2D> textureHandle(uuid);

// Resolve to actual asset (may trigger load)
Ref<Texture2D> texture = textureHandle.Get();

// Check validity
if (textureHandle.IsValid())
{
    texture->Bind(0);
}
```

---

## Properties Panel

The PropertiesPanel automatically shows properties based on selection type.

### Structure

```cpp
void PropertiesPanel::OnImGuiRender()
{
    if (!Selection::HasSelection())
    {
        ImGui::TextDisabled("Nothing selected");
        return;
    }

    Selectable primary = Selection::GetPrimarySelection();

    switch (primary.Type)
    {
        case SelectableType::Asset:
            DrawAssetProperties(primary.ID);
            break;
        case SelectableType::Entity:
            DrawEntityProperties(primary.ID);
            break;
    }
}
```

### Customizing Asset Properties

Edit `PropertiesPanel::DrawAssetProperties()` to add custom UI:

```cpp
void PropertiesPanel::DrawAssetProperties(UUID assetUUID)
{
    const AssetMetadata* metadata = AssetManager::GetRegistry().GetMetadata(assetUUID);

    // Basic info
    ImGui::Text("UUID: %llu", (uint64_t)assetUUID);
    ImGui::Text("Type: %s", AssetTypeToString(metadata->Type));
    ImGui::Text("Path: %s", metadata->FilePath.string().c_str());

    // Type-specific properties
    switch (metadata->Type)
    {
        case AssetType::Texture2D:
            // Show texture preview
            // Show import settings (sRGB, filter mode, wrap mode)
            break;

        case AssetType::Mesh:
            // Show vertex count, triangle count
            // Show submesh list
            break;

        case AssetType::Shader:
            // Show uniform list
            // Show compile status
            break;

        case AssetType::Material:
            // Show texture slots
            // Show shader properties
            break;
    }
}
```

---

## Dependency Tracking

### Forward Dependencies

Materials depend on textures. When a material is registered, its dependencies are recorded:

```cpp
AssetMetadata materialMeta;
materialMeta.Handle = materialUUID;
materialMeta.Type = AssetType::Material;
materialMeta.Dependencies = { albedoUUID, normalUUID, roughnessUUID };

AssetManager::GetRegistry().Register(materialMeta);
```

### Reverse Dependencies

The registry automatically tracks reverse dependencies:
- When Texture A is modified, find all Materials using Texture A
- Reload those Materials too

```cpp
std::vector<UUID> dependents = AssetManager::GetRegistry().GetDependents(textureUUID);
// Returns all assets that depend on this texture
```

### Cascade Reload

```cpp
void AssetManager::ReloadAsset(UUID uuid)
{
    // 1. Reload the asset itself
    auto asset = s_LoadedAssets[uuid];
    asset->Reload();

    // 2. Reload all dependents (cascade)
    auto dependents = s_Registry.GetDependents(uuid);
    for (UUID dependent : dependents)
    {
        ReloadAsset(dependent);  // Recursive
    }
}
```

---

## File Operations

### Import New Asset

```cpp
UUID uuid = AssetManager::ImportAsset("textures/new_texture.png");
// Creates .meta file, registers in registry
```

### Rename Asset

```cpp
AssetManager::RenameAsset(uuid, "renamed_texture");
// Renames file and .meta, updates registry
// UUID stays the same - all references still work
```

### Move Asset

```cpp
AssetManager::MoveAsset(uuid, "textures/subfolder");
// Moves file and .meta, updates registry
// UUID stays the same
```

### Delete Asset

```cpp
AssetManager::DeleteAsset(uuid);
// Unloads from cache
// Unregisters from registry
// Deletes file and .meta from disk
```

---

## Best Practices

1. **Always use AssetHandle for persistent references**
   - Survives hot reload
   - Lazy loading
   - Safe null checks

2. **Don't store raw asset pointers long-term**
   - Hot reload invalidates them
   - Use handles instead

3. **Keep .meta files in version control**
   - They store UUIDs
   - Without them, references break

4. **Use relative paths in asset references**
   - Portable across machines
   - Works with different project locations

5. **Don't manually edit .meta files**
   - Let the system manage them
   - Manual edits can corrupt references

---

## Troubleshooting

### Asset not loading
- Check if .meta file exists
- Verify file extension is recognized (AssetTypeFromExtension)
- Check console for error messages

### Hot reload not working
- Verify FileWatcher is started
- Check if asset has `m_IsFromFile = true`
- Ensure ProcessReloadQueue is called each frame

### Selection not showing in Properties
- Verify asset is registered in AssetRegistry
- Check Selection::HasSelection() returns true
- Ensure UUID is valid
