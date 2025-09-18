#ifndef _INC_SPI_IO_H
#define _INC_SPI_IO_H

#include <stdint.h>
#include "spi_io_conf.h"

/* Bus retcodes */
#define SPI_OK  0

int SPI_BeginTransfer(SPI_Bus bus);
int SPI_TransferData(SPI_Bus bus, uint8_t *in, const uint8_t *out, unsigned len);
void SPI_EndTransfer(SPI_Bus bus);

#endif // _INC_SPI_IO_H