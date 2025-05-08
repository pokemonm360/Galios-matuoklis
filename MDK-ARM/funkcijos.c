void ADC_SelectChannel(uint8_t channel) {
    uint8_t control_byte = 0;

    // Set control byte
    // Bits: [SGL/DIF][UNI/BIP][PD1][PD0][CH3][CH2][CH1][CH0]
    control_byte |= (1 << 7);           // SGL/DIF = 1 (single-ended mode)
    control_byte |= (1 << 6);           // UNI/BIP = 1 (unipolar mode)
    control_byte |= (channel & 0x0F);   // Select channel (CH3-CH0)

    // Send control byte via I2C
    HAL_I2C_Master_Transmit(&hi2c1, ADC_I2C_ADDRESS << 1, &control_byte, 1, I2C_TIMEOUT);
}

uint16_t ADC_ReadData(void) {
    uint8_t data[2] = {0};
    uint16_t result;

    // Receive 2 bytes from ADC
    HAL_I2C_Master_Receive(&hi2c1, ADC_I2C_ADDRESS << 1, data, 2, I2C_TIMEOUT);

    // Combine MSB and LSB into a 10-bit result
    result = ((data[0] << 8) | data[1]) >> 6;

    return result; // 10-bit ADC result
}