#pragma once

namespace Big
{\
	//Script Class
	class Script
	{
	public:
		using FuncT = void(*)();
	public:
		explicit Script(FuncT func, std::optional<std::size_t> StackSize = std::nullopt) : m_Func(func), m_ScriptFiber(nullptr), m_MainFiber(nullptr) {
			m_ScriptFiber = CreateFiber(StackSize.has_value() ? StackSize.value() : 0, [](void* param) {
				auto CurrentScript = static_cast<Script*>(param);
				CurrentScript->FiberFunc();
			}, this);
		}
		~Script() {
			if (m_ScriptFiber) DeleteFiber(m_ScriptFiber);
		}
		void Tick() {
			m_MainFiber = GetCurrentFiber();
			if (!m_WakeTime.has_value() || m_WakeTime.value() <= std::chrono::high_resolution_clock::now()) {
				SwitchToFiber(m_ScriptFiber);
			}
		}
		void ScriptYield(std::optional<std::chrono::high_resolution_clock::duration> Time = std::nullopt) {
			if (Time.has_value()) {
				m_WakeTime = std::chrono::high_resolution_clock::now() + Time.value();
			}
			else {
				m_WakeTime = std::nullopt;
			}
			SwitchToFiber(m_MainFiber);
		}
		static Script* GetCurrent() {
			return static_cast<Script*>(GetFiberData());
		}
	private:
		void FiberFunc() {
			try {
				m_Func();
			} catch (...) {}
			[]() {
			}();
			while (true) {
				ScriptYield();
			}
		}
	private:
		void* m_ScriptFiber;
		void* m_MainFiber;
		FuncT m_Func;
		std::optional<std::chrono::high_resolution_clock::time_point> m_WakeTime;
	};
	//Script Manager Class
	class ScriptManager
	{
	public:
		explicit ScriptManager() = default;
		~ScriptManager() = default;
	public:
		void AddScript(std::unique_ptr<Script> script) {
			std::lock_guard Lock(m_Mutex);
			m_Scripts.push_back(std::move(script));
		}
		void RemoveAllScripts() {
			std::lock_guard Lock(m_Mutex);
			m_Scripts.clear();
		}
		void ScriptTick() {
			static bool EnsureMainFiber = (ConvertThreadToFiber(nullptr), true);
			std::lock_guard Lock(m_Mutex);
			for (auto const& Scr : m_Scripts) {
				Scr->Tick();
			}
		}
	private:
		std::recursive_mutex m_Mutex;
		std::vector<std::unique_ptr<Script>> m_Scripts;
	};
	inline ScriptManager g_ScriptManager;
	//Fiber Pool
	extern void FiberFunc();
	class FiberPool
	{
	public:
		explicit FiberPool(std::size_t NumOfFibers = 8) {
			for (std::size_t i = 0; i < NumOfFibers; ++i) {
				g_ScriptManager.AddScript(std::make_unique<Script>(&FiberFunc));
			}
		}
		~FiberPool() = default;
		void QueueJob(std::function<void()> Func) {
			if (Func) {
				std::lock_guard lock(m_Mutex);
				m_Jobs.push(std::move(Func));
			}
		}
		void Tick() {
			std::unique_lock lock(m_Mutex);
			if (!m_Jobs.empty()) {
				auto job = std::move(m_Jobs.top());
				m_Jobs.pop();
				lock.unlock();
				std::invoke(std::move(job));
			}
		}
		std::recursive_mutex m_Mutex;
		std::stack<std::function<void()>> m_Jobs;
	};
	inline std::unique_ptr<FiberPool> g_FiberPool{};
	static void FiberFunc() {
		while (true) {
			g_FiberPool->Tick();
			Script::GetCurrent()->ScriptYield();
		}
	}
}
#define JobStart(...) Big::g_FiberPool->QueueJob([__VA_ARGS__] {
#define JobEnd });