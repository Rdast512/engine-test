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

    // Move-only (ObjectStorage is move-only).
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // --- World origin ---
    [[nodiscard]] const glm::vec3& getStartPosition() const noexcept { return startPosition_; }
    void setStartPosition(const glm::vec3& pos) noexcept { startPosition_ = pos; }

    void setBaseAxes(const glm::vec3& right, const glm::vec3& up_dir, const glm::vec3& forward) noexcept;

    // --- Owned object storage ---
    [[nodiscard]] ObjectStorage& getObjectStorage() noexcept { return objectStorage_; }
    [[nodiscard]] const ObjectStorage& getObjectStorage() const noexcept { return objectStorage_; }

private:
    glm::vec3 startPosition_{0.0f};

    // Right, Up, Forward (default = identity / world axes).


    ObjectStorage objectStorage_;
};
