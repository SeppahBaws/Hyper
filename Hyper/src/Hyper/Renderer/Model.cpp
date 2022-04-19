#include "HyperPCH.h"
// #include "Model.h"
//
// #include "Mesh.h"
//
// namespace Hyper
// {
//     Model::Model(RenderContext* pRenderCtx)
//         : m_pRenderCtx(pRenderCtx)
//         , m_Position({0.0f, 0.0f, 0.0f})
//         , m_Rotation({0.0f, 0.0f, 0.0f})
//         , m_Scale({1.0f, 1.0f, 1.0f})
//     {
//         // Dummy mesh for now.
//         m_Meshes.emplace_back(std::make_unique<Mesh>(m_pRenderCtx));
//     }
//
//     void Model::Draw(const vk::CommandBuffer& cmd) const
//     {
//         for (const std::unique_ptr<Mesh>& pMesh : m_Meshes)
//         {
//             pMesh->Draw(cmd);
//         }
//     }
// }
