#pragma once
#include "Macro/Bot.hpp"
#include <vector>

namespace scro {

// Edits the macro that is loaded in the bot. Every change can be undone.
// Rows are picked with `sel`, which always has the same length as the action list.
class Editor {
public:
    static Editor& get();

    static constexpr int kStepsPerSecond = 240;

    std::vector<char> sel;
    int page = 0;
    bool dirty = false;

    void reset();                      // forget selection and history (a different macro was loaded)
    void sync();                       // call before reading `sel`
    size_t selectedCount();
    int firstSelectedFrame();
    int lastSelectedFrame();
    void selectAll(bool value);
    void toggle(size_t index);

    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }
    void undo();
    void redo();

    void remove();                     // delete selected
    void nudge(int frames);            // move selected
    void flip();                       // press <-> release
    void swapPlayer();                 // player 1 <-> player 2
    void duplicate();                  // copy selected a little later
    void addClick(int frame);          // press and release on the jump button
    void trimBefore();                 // drop everything before the first selected input
    void trimAfter();                  // drop everything after the last selected input
    void shiftAll(int frames);         // move the whole macro
    void stripPositions();             // let physics decide instead of stored positions
    void markSaved() { dirty = false; }

private:
    void snapshot();
    void normalize();                  // keep inputs in frame order, selection follows
    std::vector<std::vector<Action>> m_undo, m_redo;
};
}
