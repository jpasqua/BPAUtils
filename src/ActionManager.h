#ifndef ActionManager_h
#define ActionManager_h

#include <stdint.h>
#include <vector>

class Action {
public:
  struct Result {
  	Result(int32_t p) : nestedActivity(nullptr), pause(p) {}
  	Result(Action* a, int32_t p) : nestedActivity(a), pause(p) {}

    Action* nestedActivity;
    int32_t pause;
  };

  virtual Result process() = 0;
  void halt() { m_started = false; }

protected:
  bool m_started = false;
};

using Actions = std::vector<Action*>;


//
// ----- PauseAction
//

class PauseAction : public Action {
public:
  PauseAction(uint32_t pause);
  virtual Action::Result process() override;

private:
  uint32_t m_paused; // How long to pause
};


//
// ----- SequenceAction
//

class SequenceAction : public Action {
public:
  SequenceAction() = default;
  SequenceAction(const Actions& actions, uint32_t pauseBetween);
  virtual Action::Result process() override;

  void setActions(const Actions& actions, uint32_t pauseBetween);
  void advance();

private:
  std::vector<Action*> m_actions;
  uint32_t m_pausedBetween;
  size_t m_index;
};


//
// ----- RepeatAction
//

class RepeatAction : public Action {
public: 
  RepeatAction(Action* action, uint32_t repeat, uint32_t pause);
  virtual Action::Result process() override;
  
private:
  Action*  m_action;      // The action to repeat
  uint32_t m_pausedAfter;  // How long to pause between repeats of the action
  uint32_t m_repeat;      // How many times to repeat the action
  uint32_t m_index;       // How far along we are in the repeat sequence
};




class ActionManager {
public:
  void begin(SequenceAction* a, bool repeatAction = false);
  void loop();
  void pause() { m_paused = true; }
  void resume() { m_paused = false; }
  void advanceMainSequence();

private:
  // ----- Private Types
  struct SuspendedAction {
    SuspendedAction() : action(nullptr), timeBeforeResuming(0) {}
    SuspendedAction(Action* a, int32_t pause) : action(a), timeBeforeResuming(pause) {}

    Action* action;
    int32_t timeBeforeResuming;
  };

  // ----- Private MemberFunctions
  SuspendedAction pop();

  // ----- Private Member Variables
  Action* m_currentAction = nullptr;
  SequenceAction* m_rootSequence = nullptr;
  bool m_repeatAction = false;
  uint32_t m_timeForNextAction = 0;
  std::vector<SuspendedAction> m_actionStack;
  bool m_paused;
};

extern ActionManager ActionMgr;
extern Action::Result ActionCompleted;

#endif // ActionManager_h

