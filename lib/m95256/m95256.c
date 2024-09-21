#include "m95256.h"
#include "m95256_port.h"

#define PAGE_ADDR_LEN   9
#define NUM_OF_PAGES    (1 << PAGE_ADDR_LEN)
#define PAGE_ADDR_MASK  (NUM_OF_PAGES - 1)
#define BYTE_ADDR_LEN   6
#define PAGE_SIZE       (1 << BYTE_ADDR_LEN)
#define BYTE_ADDR_MASK  (PAGE_SIZE - 1)

#define UPPER_QUATER_PAGE_START_ADDR    (0x6000U >> BYTE_ADDR_LEN)
#define UPPER_HALF_PAGE_START_ADDR      (0x4000U >> BYTE_ADDR_LEN)

#define CMD_WREN    0x06
#define CMD_WRDI    0x04
#define CMD_RDSR    0x05
#define CMD_WRSR    0x01
#define CMD_READ    0x03
#define CMD_WRITE   0x02

#define ST_SWRD (1 << 7)
#define ST_BP1  (1 << 3)
#define ST_BP0  (1 << 2)
#define ST_WEL  (1 << 1)
#define ST_WIP  (1 << 0)
#define NONVOLATILE_ST_BITS (ST_SWRD | ST_BP1 | ST_BP0)
#define WP_SHIFT    2
#define WP_MASK 0x03

#define WRITE_CYCLE_TIMEOUT 5

static void EnableWrite(void)
{
    S_LOW();
    SPI_TransferByte(CMD_WREN);
    S_HIGH();
}

//~ static void DisableWrite(void)
//~ {
    //~ S_LOW();
    //~ SPI_TransferByte(CMD_WRDI);
    //~ S_HIGH();
//~ }

static uint8_t ReadStatusRegister(void)
{
    uint8_t sr;

    S_LOW();
    SPI_TransferByte(CMD_RDSR);
    sr = SPI_TransferByte(0);
    S_HIGH();

    return sr;
}

static inline bool WriteCycleInProgress(void)
{
    return ReadStatusRegister() & ST_WIP;
}

static bool WriteCycleWithinTimeout(void)
{
    uint32_t startTime = MS_GET();

    while (WriteCycleInProgress()) {
        if (MS_GET() - startTime > WRITE_CYCLE_TIMEOUT)
            return false;
    }
    return true;
}

static M95_Error_t WriteStatusRegister(uint8_t value)
{
    if (W_IS_LOW())
        return M95_Error_LOCKED;
    if (!WriteCycleWithinTimeout())
        return M95_Error_TIMEOUT;

    EnableWrite();
    S_LOW();
    SPI_TransferByte(CMD_WRSR);
    SPI_TransferByte(value);
    S_HIGH();

    return M95_Error_NONE;
}

static uint16_t BytesTillEndOfPage(uint16_t byteAddr, uint16_t size)
{
    if (byteAddr + size < PAGE_SIZE)
        return size;
    else
        return PAGE_SIZE - byteAddr;
}

M95_Error_t M95256_Init(void)
{
    S_HIGH();
    H_HIGH();

    uint8_t statusReg = ReadStatusRegister();

    if (statusReg & ~NONVOLATILE_ST_BITS)
        /* On power-up only non-volatile bits can be 1. */
        return M95_Error_NONSENSE;

    if (!(statusReg & ST_SWRD))
        return WriteStatusRegister(ST_SWRD);

    return M95_Error_NONE;
}

M95_Error_t M95256_WriteArray(uint16_t addr, const uint8_t *arr, uint16_t len)
{
    uint16_t startPageAddr = addr >> BYTE_ADDR_LEN;
    uint16_t byteAddr = addr & BYTE_ADDR_MASK;
    uint16_t endPageAddr = (addr + len) >> BYTE_ADDR_LEN;
    uint16_t numOfPages = endPageAddr - startPageAddr + 1;
    uint8_t statusReg = ReadStatusRegister();
    M95_WriteProtection_t protection = statusReg >> WP_SHIFT & WP_MASK;

    if (
        protection == M95_WriteProtection_WHOLE ||
        (protection == M95_WriteProtection_HALF && endPageAddr >= UPPER_HALF_PAGE_START_ADDR) ||
        (protection == M95_WriteProtection_QUATER && endPageAddr >= UPPER_QUATER_PAGE_START_ADDR)
    ) {
        return M95_Error_LOCKED;
    }

    uint16_t currentPageAddr = startPageAddr;
    uint16_t bytesToWrite;
    uint16_t pos = 0;

    while (numOfPages--) {
        bytesToWrite = BytesTillEndOfPage(byteAddr, len);

        if (!WriteCycleWithinTimeout())
            return M95_Error_TIMEOUT;

        EnableWrite();
        S_LOW();
        SPI_TransferByte(CMD_WRITE);
        SPI_TransferByte(addr >> 8);
        SPI_TransferByte((uint8_t)addr);
        SPI_TransferBytes(NULL, arr + pos, bytesToWrite);
        S_HIGH();

        // If write cycle didn't start it is very strange.
        if (!WriteCycleInProgress())
            return M95_Error_NONSENSE;

        currentPageAddr++;
        byteAddr = 0;
        addr = currentPageAddr << BYTE_ADDR_LEN;
        len -= bytesToWrite;
        pos += bytesToWrite;
    }
    return M95_Error_NONE;
}

M95_Error_t M95256_ReadArray(uint16_t addr, uint8_t *arr, uint16_t len)
{
    if (!WriteCycleWithinTimeout())
        return M95_Error_TIMEOUT;

    uint8_t header[3] = {CMD_READ, addr >> 8, (uint8_t)addr};

    S_LOW();
    SPI_TransferBytes(NULL, header, sizeof(header));
    SPI_TransferBytes(arr, NULL, len);
    S_HIGH();

    return M95_Error_NONE;
}

M95_Error_t M95256_SetWriteProtection(M95_WriteProtection_t protection)
{
    return WriteStatusRegister(ST_SWRD | protection << WP_SHIFT);
}
