// #pragma once
// #include <glm/vec3.hpp>
//
// #include "RenderContext.h"
//
// namespace Hyper
// {
//     class Mesh;
//
//     class Model
//     {
//     public:
//         explicit Model(RenderContext* pRenderCtx);
//         // void LoadFrom(const std::filesystem::path& filePath);
//
//         void Draw(const vk::CommandBuffer& cmd) const;
//
//     private:
//         RenderContext* m_pRenderCtx{};
//
//         glm::vec3 m_Position{};
//         glm::vec3 m_Rotation{};
//         glm::vec3 m_Scale{};
//
//         std::vector<std::unique_ptr<Mesh>> m_Meshes{};
//     };
// }
