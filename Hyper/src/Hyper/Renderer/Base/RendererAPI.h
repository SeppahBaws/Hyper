#pragma once

namespace Hyper
{
	class RendererAPI
	{
	public:
		RendererAPI() = default;
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;

		virtual void BeginScene() = 0;
		virtual void EndScene() = 0;

		virtual void Clear() = 0;

		virtual void DrawIndexed() = 0;
	};
}
