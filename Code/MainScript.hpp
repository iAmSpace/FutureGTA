#pragma once
#include "Script.hpp"

namespace Big
{
	class MainScript {
	public:
		explicit MainScript() = default;
		~MainScript() noexcept = default;
		void Init();
		static void Tick();
	};
	inline std::shared_ptr<MainScript> g_MainScript;
}