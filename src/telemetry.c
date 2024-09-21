#include "telemetry.h"
#include "mcu.h"
#include "uart.h"

void Telemetry_Send(struct TelemetryControlBlock *ctlb)
{
    if (ctlb->write == NULL || ctlb->enabled == false)
        return;

    if (Millis_Get() - ctlb->_lastTime >= ctlb->interval) {
        static char telemetryString[TELEMETRY_STRING_SIZE];

        ctlb->write(telemetryString);
        UART_SendString(telemetryString);
        ctlb->_lastTime = Millis_Get();
    }
}
