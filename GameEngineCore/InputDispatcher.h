#ifndef GAME_ENGINE_INPUT_DISPATCHER_H
#define GAME_ENGINE_INPUT_DISPATCHER_H

#include "HardwareInputInterface.h"

namespace GameEngine
{
	enum class MouseButton
	{
		None, Left, Middle, Right
	};
	struct MouseInput
	{
		MouseButton Button;
		int X, Y, Delta;
	};
	typedef CoreLib::Func<bool, const CoreLib::String &, float> ActionInputHandlerFunc;
	typedef CoreLib::Func<bool, const CoreLib::String &, MouseInput> ActorMouseInputHandlerFunc;

	struct InputMappingValue
	{
		CoreLib::String ActionName;
		float Value = 1.0f;
	};

	class InputDispatcher
	{
	private:
		CoreLib::RefPtr<HardwareInputInterface> inputInterface;
		CoreLib::EnumerableDictionary<wchar_t, InputMappingValue> keyboardAxisMappings;
		CoreLib::EnumerableDictionary<wchar_t, InputMappingValue> keyboardActionMappings;
		CoreLib::Dictionary<CoreLib::String, CoreLib::List<ActionInputHandlerFunc>> actionHandlers;
		CoreLib::List<ActorMouseInputHandlerFunc> mouseActionHandlers;
	public:
		InputDispatcher(const CoreLib::RefPtr<HardwareInputInterface> & pInputInterface);
		void LoadMapping(const CoreLib::String & fileName);
		void BindActionHandler(const CoreLib::String & axisName, ActionInputHandlerFunc handlerFunc);
		void UnbindActionHandler(const CoreLib::String & axisName, ActionInputHandlerFunc handlerFunc);
		void BindMouseInputHandler(ActorMouseInputHandlerFunc handlerFunc);
		void UnbindMouseInputHandler(ActorMouseInputHandlerFunc handlerFunc);
		void DispatchInput();
		void DispatchAction(CoreLib::String actionName, float actionValue = 1.0f);
	};
}

#endif