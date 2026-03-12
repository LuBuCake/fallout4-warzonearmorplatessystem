#pragma once

namespace Hooks
{
	void ProcessPlates(RE::Actor* actor, RE::HitData& hitData);
	void ProcessPlatesLoot(RE::Actor* actor, RE::HitData& hitData);
	void Hook_ProcessHitData(RE::Actor* actor, RE::HitData& hitData);

	class MenuWatcher : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		void Initialize();

	public:
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;
	};

	class SubgraphWatcher
	{
	public:
		typedef RE::BSEventNotifyControl(SubgraphWatcher::* OriginalProcessEventFunction)(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);
		RE::BSEventNotifyControl ProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);
		void Sink();

	protected:
		static std::unordered_map<uintptr_t, OriginalProcessEventFunction> FunctionMap;
	};

	void Initialize();
	void InitializeOnLaunch();
}