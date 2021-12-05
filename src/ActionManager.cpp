#include <Arduino.h>
#include "ActionManager.h"


//
// ----- SequenceAction
//

SequenceAction::SequenceAction(const Actions& actions, uint32_t pauseBetween)
  : m_actions(actions), m_pauseBetween(pauseBetween) { }

Action::Result SequenceAction::process() {
  // Log.verbose("SequenceAction::process");
  if (!m_started) {
    m_index = 0;
    m_started = true;
  }
  if (m_index >= m_actions.size()) {
    m_started = false;
    return ActionCompleted;
  }
  return Result(m_actions[m_index++], m_pauseBetween);
}

void SequenceAction::setActions(const Actions& actions, uint32_t pauseBetween) {
  m_actions = actions;
  m_pauseBetween = pauseBetween;
}


//
// ----- PauseAction
//

PauseAction::PauseAction(uint32_t pause) : m_pause(pause) {}

Action::Result PauseAction::process()  {
  // Log.verbose("PauseAction::process");
  // We only pause once, so if we're already started, just return
  if (m_started) {
    m_started = false;
    return ActionCompleted;
  } else {
    m_started = true;
    return Result(m_pause);
  }
}


//
// ----- RepeatAction
//

RepeatAction::RepeatAction(Action* action, uint32_t repeat, uint32_t pause)
    : m_action(action), m_pauseAfter(pause), m_repeat(repeat), m_index(0)
  { }

Action::Result RepeatAction::process() {
  // Log.verbose("RepeatAction::process");
  if (!m_started) {
    m_index = 0;
    m_started = true;
  }

  if (m_index < m_repeat) {
    m_started = true;
    m_index++;
    // Log.verbose("RepeatAction::process, returning nested action");
    return Result(m_action, m_pauseAfter);
  }

  m_started = false;
  return ActionCompleted; // Nothing left to do
}


//
// ----- ActionManager
//

void ActionManager::begin(Action* a, bool repeatAction) {
	m_currentAction = m_rootAction = a;
	m_repeatAction = repeatAction;
	m_pauseUntil = 0;
}

void ActionManager::loop() {
	if (m_pause) return;
	if (millis() < m_pauseUntil) return;

	if (m_currentAction == nullptr) {
	  auto paused = pop();
	  if (paused.action)  m_currentAction = paused.action;
	  else if (m_repeatAction && m_rootAction) m_currentAction = m_rootAction;
	  else { m_currentAction = nullptr; return; }
	  m_pauseUntil = millis() + paused.timeBeforeResuming;
	}

	auto result = m_currentAction->process();
	if (result.newActivity != nullptr) {
	  m_actionStack.push_back(PausedAction(m_currentAction, result.pause));
	  m_currentAction = result.newActivity;
	  m_pauseUntil = 0;
	} else if (result.pause < 0) {
	  m_currentAction = nullptr;
	  m_pauseUntil = 0;
	} else m_pauseUntil = millis() + result.pause;
}

ActionManager::PausedAction ActionManager::pop() {
  PausedAction p;
  if (m_actionStack.size()) {
		p = m_actionStack.back();
		m_actionStack.pop_back();
  }
  return p;
}

Action::Result ActionCompleted(-1);
ActionManager ActionMgr;

