#include <Arduino.h>
#include "ActionManager.h"


//
// ----- SequenceAction
//

SequenceAction::SequenceAction(const Actions& actions, uint32_t pauseBetween)
  : m_actions(actions), m_pausedBetween(pauseBetween) { }

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
  return Result(m_actions[m_index++], m_pausedBetween);
}

void SequenceAction::setActions(const Actions& actions, uint32_t pauseBetween) {
  m_actions = actions;
  m_pausedBetween = pauseBetween;
}

void SequenceAction::advance() {
  m_index++;
  if (m_index >= m_actions.size())  m_index = 0;
}

//
// ----- PauseAction
//

PauseAction::PauseAction(uint32_t pause) : m_paused(pause) {}

Action::Result PauseAction::process()  {
  // Log.verbose("PauseAction::process");
  // We only pause once, so if we're already started, just return
  if (m_started) {
    m_started = false;
    return ActionCompleted;
  } else {
    m_started = true;
    return Result(m_paused);
  }
}


//
// ----- RepeatAction
//

RepeatAction::RepeatAction(Action* action, uint32_t repeat, uint32_t pause)
    : m_action(action), m_pausedAfter(pause), m_repeat(repeat), m_index(0)
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
    return Result(m_action, m_pausedAfter);
  }

  m_started = false;
  return ActionCompleted; // Nothing left to do
}


//
// ----- ActionManager
//

void ActionManager::begin(SequenceAction* a, bool repeatAction) {
	m_currentAction = m_rootSequence = a;
	m_repeatAction = repeatAction;
	m_timeForNextAction = 0;
}

void ActionManager::loop() {
	if (m_paused) return;
	if (millis() < m_timeForNextAction) return;

	if (m_currentAction == nullptr) {
	  auto paused = pop();
	  if (paused.action)  m_currentAction = paused.action;
	  else if (m_repeatAction && m_rootSequence) m_currentAction = m_rootSequence;
	  else { m_currentAction = nullptr; return; }
	  m_timeForNextAction = millis() + paused.timeBeforeResuming;
	}

	auto result = m_currentAction->process();
	if (result.nestedActivity != nullptr) {
	  m_actionStack.push_back(SuspendedAction(m_currentAction, result.pause));
	  m_currentAction = result.nestedActivity;
	  m_timeForNextAction = 0;
	} else if (result.pause < 0) {
	  m_currentAction = nullptr;
	  m_timeForNextAction = 0;
	} else m_timeForNextAction = millis() + result.pause;
}

ActionManager::SuspendedAction ActionManager::pop() {
  SuspendedAction p;  // A default object where action is nullptr

  if (m_actionStack.size()) {
		p = m_actionStack.back();
		m_actionStack.pop_back();
  }
  return p;
}

void ActionManager::advanceMainSequence() {
  if (m_rootSequence == nullptr) return;
  if (m_currentAction == nullptr) m_currentAction = pop().action;

  while (m_currentAction != m_rootSequence) {
    m_currentAction->halt();
    m_currentAction = pop().action;
  }

  m_rootSequence->advance();
}

Action::Result ActionCompleted(-1);
ActionManager ActionMgr;

