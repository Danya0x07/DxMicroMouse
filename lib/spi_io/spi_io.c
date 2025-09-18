#include "spi_io.h"
#include "spi_io_port.h"

int SPI_BeginTransfer(SPI_Bus bus)
{
    const struct SPI_BusCtl *busCtl = &BUSES[bus];
    return busCtl->begin();
}

int SPI_TransferData(SPI_Bus bus, uint8_t *in, const uint8_t *out, unsigned len)
{
    const struct SPI_BusCtl *busCtl = &BUSES[bus];
    int retcode = SPI_OK;

    if (in == NULL && out != NULL) {
        uint8_t tmp;
        while (retcode == SPI_OK && len--)
            retcode = busCtl->transfer(&tmp, *out++);
    }
    else if (in != NULL && out == NULL) {
        while (retcode == SPI_OK && len--)
            retcode = busCtl->transfer(in++, 0);
    }
    else if (in != NULL && out != NULL) {
        while (retcode == SPI_OK && len--)
            retcode = busCtl->transfer(in++, *out++);
    }
    else {
        uint8_t tmp;
        while (retcode == SPI_OK && len--)
            retcode = busCtl->transfer(&tmp, 0);
    }

    return retcode;
}

void SPI_EndTransfer(SPI_Bus bus)
{
    const struct SPI_BusCtl *busCtl = &BUSES[bus];
    busCtl->end();
}