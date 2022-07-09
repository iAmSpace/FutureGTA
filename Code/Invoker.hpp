#pragma once
#include "Game.hpp"
#include "Types.hpp"
#include "Crossmap.hpp"

namespace rage {
	struct scrNativeCallContext {
	protected:
		void* m_Return;
		std::uint32_t m_ArgCount;
		char padding[4];
		void* m_Args;
		std::uint32_t m_DataCount;
		char padding1[4];
		alignas(std::uintptr_t) std::uint8_t m_VectorSpace[192];
	};
	struct scrNativeRegistration {
		std::uint64_t m_NextRegistration1;
		std::uint64_t m_NextRegistration2;
		void* m_Handlers[7];
		std::uint32_t m_NumEntries1;
		std::uint32_t m_NumEntries2;
		std::uint64_t m_Hashes;
		inline scrNativeRegistration* GetNextRegistration() {
			std::uintptr_t result;
			auto v5 = std::uintptr_t(&m_NextRegistration1); auto v12 = 2;
			auto v13 = v5 ^ m_NextRegistration2; auto v14 = (char *)&result - v5;
			do {
				*(DWORD*)&v14[v5] = static_cast<DWORD>(v13) ^ *(DWORD*)v5;
				v5 += 4; --v12;
			} while (v12);
			return reinterpret_cast<scrNativeRegistration*>(result);
		}
		inline std::uint32_t GetNumEntries() {
			return static_cast<std::uint32_t>(((std::uintptr_t)&m_NumEntries1) ^ m_NumEntries1 ^ m_NumEntries2);
		}
		inline std::uint64_t GetHash(std::uint32_t index) {
			auto naddr = 16 * index + std::uintptr_t(&m_NextRegistration1) + 0x54; auto v8 = 2;
			std::uint64_t nResult; auto v11 = (char*)&nResult - naddr;
			auto v10 = naddr ^ *(DWORD*)(naddr + 8);
			do {
				*(DWORD *)&v11[naddr] = static_cast<DWORD>(v10 ^ *(DWORD*)(naddr));
				naddr += 4; --v8;
			} while (v8);
			return nResult;
		}
	};
	class scrNativeRegistrationTable {
		scrNativeRegistration* m_entries[0xFF];
		std::uint32_t m_unk;
		bool m_initialized;
	};
	using scrNativeHash = std::uint64_t;
	using scrNativeMapping = std::pair<scrNativeHash, scrNativeHash>;
	using scrNativeHandler = void(*)(scrNativeCallContext*);
}
namespace Big
{
	struct NativeContext : public rage::scrNativeCallContext {
		NativeContext() {
			Reset();
		}
		void Reset() {
			m_Return = &m_ReturnData[0];
			m_ArgCount = 0;
			m_Args = &m_ArgumentData[0];
			m_DataCount = 0;
			std::fill(std::begin(m_VectorSpace), std::end(m_VectorSpace), '\0');
			std::fill(std::begin(m_ReturnData), std::end(m_ReturnData), 0);
			std::fill(std::begin(m_ArgumentData), std::end(m_ArgumentData), 0);
		}
		template <typename T>
		void Push(T value) {
			static_assert(sizeof(T) <= 8);
			std::uintptr_t data{};
			*(T*)(&data) = value;
			m_ArgumentData[m_ArgCount++] = data;
		}
		template <>
		void Push<NativeVector3>(NativeVector3 value) {
			Push(value.x);
			Push(value.y);
			Push(value.z);
		}
		template <typename T>
		T GetReturnValue() { return *static_cast<T*>(m_Return); }
		template <>
		void GetReturnValue<void>() {}
	private:
		std::uintptr_t m_ReturnData[10] = {};
		std::uintptr_t m_ArgumentData[100] = {};
	};
	class Invoker {
	public:
		explicit Invoker() = default;
		~Invoker() noexcept = default;
		Invoker(Invoker const&) = delete;
		Invoker(Invoker&&) = delete;
		Invoker& operator=(Invoker const&) = delete;
		Invoker& operator=(Invoker&&) = delete;
		bool MapNatives() {
			for (auto const& mapping : g_Crossmap) {
				rage::scrNativeHandler handler = g_GameFunctions->m_GetNativeHandler(
					g_GameVariables->m_NativeRegistrationTable, mapping.m_New);
				m_Cache.emplace(mapping.m_Old, handler);
			}
			return false;
		}
		void BeginCall() { m_Context.Reset(); }
		template <typename T> void Push(T&& value) { m_Context.Push(std::forward<T>(value)); }
		template <typename T> T GetReturn() { return m_Context.GetReturnValue<T>(); }
		void EndCall(std::uint64_t hash)
		{
			if (auto cache = m_Cache.find(hash); cache != m_Cache.end()) {
				rage::scrNativeHandler handler = cache->second;
				if (!handler) {
					g_Logger->Info("Failed to invoke native %p", hash);
					return;
				}
				handler(&m_Context);
				g_GameFunctions->m_FixVectors(&m_Context);
			}
			else {
				g_Logger->Error("Failed to find 0x%p native's handler.", hash);
			}
		}
	private:
		NativeContext m_Context;
		std::unordered_map<rage::scrNativeHash, rage::scrNativeHandler> m_Cache;
	};
	inline std::unique_ptr<Invoker> g_Invoker;
}