#include "quadspi.h"

extern QSPI_HandleTypeDef hqspi;

static uint8_t QSPI_WriteEnable(void);
static uint8_t QSPI_AutoPollingMemReady(void);
static uint8_t QSPI_ResetChip(void);


uint8_t QSPI_ResetChip(void) {

    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_ENABLE_RESET,
    };

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q64JV_RESET_DEVICE;
    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    if (QSPI_AutoPollingMemReady() != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t QSPI_AutoPollingMemReady(void) {

    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_STATUS_REG1,
        .DataMode = QSPI_DATA_1_LINE,
    };

    QSPI_AutoPollingTypeDef conf = {
        .Match = 0x00,
        .Mask = 0x01,
        .MatchMode = QSPI_MATCH_MODE_AND,
        .StatusBytesSize = 1,
        .Interval = 0x10,
        .AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE,
    };

    if (HAL_QSPI_AutoPolling(&hqspi, &cmd, &conf, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t QSPI_WriteEnable(void) {

    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_WRITE_ENABLE,
    };

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = W25Q64JV_STATUS_REG1;

    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.DummyCycles = 0;
    cmd.NbData = 0;

    QSPI_AutoPollingTypeDef conf = {
        .Match = 0x02,
        .Mask = 0x02,
        .MatchMode = QSPI_MATCH_MODE_AND,
        .StatusBytesSize = 1,
        .Interval = 0x10,
        .AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE,
    };

    if (HAL_QSPI_AutoPolling(&hqspi, &cmd, &conf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t CSP_QUADSPI_Init(void) {
    hqspi.Instance = QUADSPI;
    if (HAL_QSPI_DeInit(&hqspi) != HAL_OK) {
        return HAL_ERROR;
    }
    MX_QUADSPI_Init();
    if (QSPI_ResetChip() != HAL_OK) {
        return HAL_ERROR;
    }
    HAL_Delay(1);
    
    if (QSPI_AutoPollingMemReady() != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress, uint32_t EraseEndAddress) {
    EraseStartAddress = EraseStartAddress - EraseStartAddress % MEMORY_SECTOR_SIZE;
    
    if (QSPI_WriteEnable() != HAL_OK) {
        return HAL_ERROR;
    }

    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_ERASE_SECTOR,
        .AddressMode = QSPI_ADDRESS_1_LINE,
        .AddressSize = QSPI_ADDRESS_24_BITS,
    };
    while (EraseEndAddress >= EraseStartAddress) {
        cmd.Address = (EraseStartAddress & 0x0FFFFFFF);
        if (QSPI_WriteEnable() != HAL_OK) {
            return HAL_ERROR;
        }

        if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }
        EraseStartAddress += MEMORY_SECTOR_SIZE;
        if (QSPI_AutoPollingMemReady() != HAL_OK) {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

uint8_t CSP_QSPI_WriteMemory(uint8_t *buffer, uint32_t address, uint32_t buffer_size) {
    uint32_t end_addr, current_size, current_addr;
    current_addr = 0;
    
    while (current_addr <= address) {
        current_addr += MEMORY_PAGE_SIZE;
    }
    current_size = current_addr - address;
    if (current_size > buffer_size) {
        current_size = buffer_size;
    }
    current_addr = address;
    end_addr = address + buffer_size;
    
    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_PAGE_PROGRAM,
        .AddressMode = QSPI_ADDRESS_1_LINE,
        .AddressSize = QSPI_ADDRESS_24_BITS,
        .DataMode = QSPI_DATA_1_LINE,
        .DummyCycles = 0,
    };
    do {
        cmd.Address = current_addr;
        cmd.NbData = current_size;
        
        if (current_size == 0) {
            return HAL_OK;
        }
        
        if (QSPI_WriteEnable() != HAL_OK) {
            return HAL_ERROR;
        }
        if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }
        
        if (HAL_QSPI_Transmit(&hqspi, buffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return HAL_ERROR;
        }
        if (QSPI_AutoPollingMemReady() != HAL_OK) {
            return HAL_ERROR;
        }
        current_addr += current_size;
        buffer += current_size;
        current_size = ((current_addr + MEMORY_PAGE_SIZE) > end_addr) ?
                                (end_addr - current_addr) : MEMORY_PAGE_SIZE;
    } while (current_addr <= end_addr);
    return HAL_OK;
}

uint8_t CSP_QSPI_EnableMemoryMappedMode(void) {
    QSPI_MemoryMappedTypeDef mem_mapped_cfg = {
        .TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE,
    };

    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_INPUT_FAST_READ,
        .AddressMode = QSPI_ADDRESS_4_LINES,
        .Address = 0,
        .AddressSize = QSPI_ADDRESS_24_BITS,
        .AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES,
        .AlternateBytesSize= QSPI_ALTERNATE_BYTES_8_BITS,
        .AlternateBytes    = 0xf0, //datasheet p22
        .DataMode = QSPI_DATA_4_LINES,
        .DummyCycles = 4,
        .NbData = 0,
    };
    if (HAL_QSPI_MemoryMapped(&hqspi, &cmd, &mem_mapped_cfg) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

uint8_t CSP_QSPI_Erase_Chip(void) {
    if (QSPI_WriteEnable() != HAL_OK) {
        return HAL_ERROR;
    }
    QSPI_CommandTypeDef cmd = {
        .InstructionMode = QSPI_INSTRUCTION_1_LINE,
        .Instruction = W25Q64JV_ERASE_CHIP,
    };

    if (HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }
    if (QSPI_AutoPollingMemReady() != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
