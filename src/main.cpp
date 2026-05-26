#define SDL_MAIN_HANDLED
#include "core/runtime_state.h"
#include "net/zmq_receiver.h"
#include "gui_app.h"
#include <thread>

int main() {
    static RuntimeState state;

    std::thread serverThread(RunZmqReceiver, &state, 5558, 5559, true);

    if (GuiAppInit(1400, 900, "Backend Control")) {
        while (GuiAppFrame(state)) {}
        GuiAppShutdown();
    }

    state.running.store(false);
    serverThread.join();
    return 0;
}
