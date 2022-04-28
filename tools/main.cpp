#include "exec_graph.h"
#include "xpti_trace_framework.h"

int main() {
    ExecGraph asd;
    uint16_t eventType;
    xpti::trace_event_data_t *event;
    asd.modifyExecGraph(eventType, event);

    return 0;
}
