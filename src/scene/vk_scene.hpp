#pragma once

#include "../core/object_storage.hpp"

#include <array>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Scene - top-level container that owns the object storage, a world-space
//         origin, and the base orientation axes for the scene.
// ---------------------------------------------------------------------------
class Scene
{
public:
    Scene() = default;
    ~Scene() = default;

    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // --- World origin (mutable: accessors) ---
    [[nodiscard]] const glm::vec3& getStartPosition() const noexcept { return startPosition; }
    void setStartPosition(const glm::vec3& pos) noexcept { startPosition = pos; }

    void setBaseAxes(const glm::vec3& right, const glm::vec3& upDir, const glm::vec3& forward) noexcept;

    // --- Owned storage (direct access) ---
    // ObjectStorage is not defined yet; placeholder for scene graph work.
    // ObjectStorage objectStorage;

private:
    glm::vec3 startPosition{0.0f};
};
