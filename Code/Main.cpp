#include "Game.hpp"
#include "Hooking.hpp"
#include "Script.hpp"
#include "LogScript.hpp"
#include "MainScript.hpp"
#include "Invoker.hpp"
#include "CustomText.hpp"
#include "D3DRenderer.hpp"
#include "UI/UIManager.hpp"
#include "Translation.hpp"

BOOL DllMain(HINSTANCE hInstance, DWORD reason, LPVOID) {
	using namespace Big;

	if (reason == DLL_PROCESS_ATTACH) {
		g_Module = hInstance;
		CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
			g_Logger = std::make_unique<Logger>();
			g_Logger->Info("BigBase injected.");
			g_TranslationManager = std::make_unique<TranslationManager>();
			//g_TranslationManager->LoadTranslations("English");
			g_GameFunctions = std::make_unique<GameFunctions>();
			g_GameVariables = std::make_unique<GameVariables>();
			//Wait for the game to load
			while (*g_GameVariables->m_GameState != 0) {
				std::this_thread::sleep_for(3s);
				std::this_thread::yield();
			}
			g_GameVariables->PostInit();
			g_Invoker = std::make_unique<Invoker>();
			g_CustomText = std::make_unique<CustomText>();
			g_D3DRenderer = std::make_unique<D3DRenderer>();
			g_UiManager = std::make_unique<UserInterface::UIManager>();
			g_MainScript = std::make_shared<MainScript>();
			g_LogScript = std::make_shared<LogScript>();
			g_FiberPool = std::make_unique<FiberPool>();
			g_ScriptManager.AddScript(std::make_unique<Script>(&LogScript::Tick));
			g_ScriptManager.AddScript(std::make_unique<Script>(&MainScript::Tick));
			g_Hooking = std::make_unique<Hooking>();
			g_Hooking->Hook();
			g_Invoker->MapNatives();
			g_Logger->Info("Cached all natives!");
			g_Logger->Info("BigBase loaded.");
			while (g_Running) {
				if (GetAsyncKeyState(VK_END)) g_Running = false;
				std::this_thread::sleep_for(3ms);
				std::this_thread::yield();
			}
			std::this_thread::sleep_for(500ms);
			g_Hooking->Unhook();
			g_LogScript.reset();
			g_MainScript.reset();
			g_FiberPool.reset();
			g_ScriptManager.RemoveAllScripts();
			g_UiManager.reset();
			g_UiManager.reset();
			g_D3DRenderer.reset();
			g_CustomText.reset();
			g_Invoker.reset();
			std::this_thread::sleep_for(500ms);
			g_Hooking.reset();
			g_TranslationManager.reset();
			g_GameVariables.reset();
			g_GameFunctions.reset();		
			g_Logger->Info("BigBase unloaded.");
			g_Logger.reset();
			FreeLibraryAndExitThread(g_Module, 0);
		}, nullptr, 0, nullptr);
	}
	return true;
}