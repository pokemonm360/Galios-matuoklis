/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "u8g2.h"
#include "u8g2_stm32f4.c"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//#define DEBUG

typedef struct { //Struktura saugojimui i flash atminti
  uint8_t  K1, K2;
  uint16_t MAT, OLED, PC;
  uint8_t  addr1, addr2;
} Defaults_t;

#define I2C_TIMEOUT 50

#define NUMBER_OF_CHANNELS 2 //Kiek kanalu naudojame
uint8_t INA219_1_ADDRESS = 0; //Pirmojo INA219 adresas
uint8_t INA219_2_ADDRESS = 0; //Antrojo INA219 adresas

#define CONFIG_REG      0x00 //INA219 konfiguracijos registro adresas
#define CALIBRATION_REG 0x05 //INA219 kalibracijos registro adresas
#define BUS_VOLTAGE_REG 0x02 //INA219 ismatuotos itampos registro adresas
#define CURRENT_REG     0x04 //INA219 ismatuotos sroves registro adresas

uint16_t default_PC = 0; //Kas kiek laiko siusti duomenis i PC, po mygtuko nuspaudimo (galima keisti, irasant nauja verte i flash)
uint16_t default_OLED = 0; //Kas kiek laiko siusti duomenis i OLED, po mygtuko nuspaudimo (galima keisti, irasant nauja verte i flash)
uint16_t default_MATAVIMAS = 0; //Kas kiek laiko atlikti matavimus, po mygtuko nuspaudimo (galima keisti, irasant nauja verte i flash)

uint8_t default_KIEKIS1 = 0; //Kiek kartu vidurkinant automatiskai matuoti pacio INA219 viduje, po mygtuko nuspaudimo (galima keisti, irasant nauja verte i flash)
uint8_t default_KIEKIS2 = 0;

#define TIMER_INTERRUPT_TIME 250 //Kaip daznai ivyksta laikmacio pertrauktis

#define UART_ZINUTES_ILGIS 56  //Nustato priimamos UART zinutes ilgi

uint8_t RxBuferis[UART_ZINUTES_ILGIS]; //Buferis UART zinutei

uint8_t STOP = 1; //Nurodo ar procesorius turi matuoti galias ir vykdyti programa, kai STOP = 1, programa neveikia

uint8_t PC_laikas = 0; //Nustato, kiek laikmacio petraukciu praeina, kol reikia siusti i PC
uint8_t OLED_laikas = 0; //Nustato, kiek laikmacio petraukciu praeina, kol reikia siusti i OLED
uint8_t MATAVIMO_laikas = 0; //Nustato, kiek laikmacio petraukciu praeina, kol reikia matuoti itampas ir sroves

uint8_t PC_indeksas = 0; //Seka, kiek matavimu turejo ir lygina su PC_laikas
uint8_t OLED_indeksas = 0; //Seka, kiek matavimu turejo ir lygina su OLED_laikas
uint8_t MATAVIMO_indeksas = 0; //Seka, kiek laikmacio petraukciu turejo ir lygina su MATAVIMO_laikas

uint8_t config_byte[2]; //konfiguracijos baito masyvas irasymui i INA219
uint8_t calib_byte[2]; //kalibracijos baito masyvas irasymui i INA219

float ISMATUOTA_ITAMPA_OLED[20][NUMBER_OF_CHANNELS]; //Ismatuotos itampos masyvas atvaizdavimui i OLED
float ISMATUOTA_SROVE_OLED[20][NUMBER_OF_CHANNELS]; //Ismatuotos sroves masyvas atvaizdavimui i OLED
float ISMATUOTA_ITAMPA_PC[20][NUMBER_OF_CHANNELS]; //Ismatuotos itampos masyvas atvaizdavimui i PC
float ISMATUOTA_SROVE_PC[20][NUMBER_OF_CHANNELS]; //Ismatuotos sroves masyvas atvaizdavimui i PC
double ITAMPOS_VIDURKIS_OLED[NUMBER_OF_CHANNELS] = {0}; //Itampos vidurkis atvaizdavimui i OLED
double SROVES_VIDURKIS_OLED[NUMBER_OF_CHANNELS] = {0}; //Sroves vidurkis atvaizdavimui i OLED
double ITAMPOS_VIDURKIS_PC[NUMBER_OF_CHANNELS] = {0}; //Itampos vidurkis atvaizdavimui i PC
double SROVES_VIDURKIS_PC[NUMBER_OF_CHANNELS] = {0}; //Sroves vidurkis atvaizdavimui i PC

uint8_t KIEKIS1 = 0; //Nurodo, kiek kartu automatiskai ismatuoti srove/itampa is eiles, taip sumazinant triuksma 1 kanalui
uint8_t KIEKIS2 = 0; //Nurodo, kiek kartu automatiskai ismatuoti srove/itampa is eiles, taip sumazinant triuksma 2 kanalui
uint16_t MATAVIMAS = 0; //Nurodo, kas kiek ms atlikti matavima
uint16_t OLED = 0; //Nurodo, kas kiek ms atlikti atvaizdavima OLED
uint16_t PC = 0; //Nurodo, kas kiek ms atlikti atvaizdavima PC

uint16_t bus_voltage; //Ismatuota raw verte is INA219 itampos registro
int16_t current; //Ismatuota raw verte is INA219 sroves registro

uint8_t bus_voltage_reg[2]; //Ismatuotos raw vertes is INA219 itampos registro masyvas
uint8_t current_reg[2]; //Ismatuotos raw vertes is INA219 sroves registro masyvas

uint8_t config_final[4] = {0}; //Finaline konfiguracijos verte, ji apskaiciuojama viena karta ir jos irasymas kiekviena karta paleidzia itampos ir sroves matavima

char buf[60];  //Reikalinga UART siuntimui is MV i PC
uint8_t len; //Reikalinga UART siuntimui is MV i PC

extern uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr); //Reikia OLED ekranui
extern uint8_t u8x8_byte_stm32_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr); //Reikia OLED ekranui
static u8g2_t u8g2; //Reikia OLED ekranui

#include "stm32l0xx_hal_flash_ex.h"  // Reikalinga flash atminties rasymui
#include "string.h"

#define FLASH_USER_PAGE_ADDR   (FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE)

void SaveDefaultsToFlash(void) {
    FLASH_EraseInitTypeDef EraseInit = {0};
    uint32_t PageError;

    // 1) Unlock & clear flags
    HAL_FLASH_Unlock();
    HAL_UART_Transmit(&huart2, (uint8_t*)"FLASH Unlock\r\n", strlen("FLASH Unlock\r\n"), 100);
    __HAL_FLASH_CLEAR_FLAG(
        FLASH_FLAG_EOP    |
        FLASH_FLAG_WRPERR |
        FLASH_FLAG_PGAERR
    );
    HAL_UART_Transmit(&huart2, (uint8_t*)"Flags cleared\r\n", strlen("Flags cleared\r\n"), 100);

    // 2) Erase page
    EraseInit.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInit.PageAddress = FLASH_USER_PAGE_ADDR;
    EraseInit.NbPages     = 1;
    HAL_UART_Transmit(&huart2, (uint8_t*)"Erasing page...\r\n", strlen("Erasing page...\r\n"), 100);

    if (HAL_FLASHEx_Erase(&EraseInit, &PageError) == HAL_OK) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Erase OK\r\n", strlen("Erase OK\r\n"), 100);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Erase ERR\r\n", strlen("Erase ERR\r\n"), 100);
    }

    // 3) Pack your defaults into 3 words
    uint32_t word0 =  (uint32_t)default_KIEKIS1
                    | ((uint32_t)default_KIEKIS2 << 8)
                    | ((uint32_t)default_MATAVIMAS << 16);
    uint32_t word1 =  (uint32_t)default_OLED
                    | ((uint32_t)default_PC << 16);
    uint32_t word2 =  (uint32_t)INA219_1_ADDRESS
                    | ((uint32_t)INA219_2_ADDRESS << 8);
    uint32_t addr = FLASH_USER_PAGE_ADDR;

    // 4) Program word0
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, word0) == HAL_OK) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Prog0 OK\r\n", strlen("Prog0 OK\r\n"), 100);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Prog0 ERR\r\n", strlen("Prog0 ERR\r\n"), 100);
    }

    // 5) Program word1
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + 4, word1) == HAL_OK) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Prog1 OK\r\n", strlen("Prog1 OK\r\n"), 100);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Prog1 ERR\r\n", strlen("Prog1 ERR\r\n"), 100);
    }

    // 6) Program word2
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + 8, word2) == HAL_OK) {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Prog2 OK\r\n", strlen("Prog2 OK\r\n"), 100);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Prog2 ERR\r\n", strlen("Prog2 ERR\r\n"), 100);
    }

    // 7) Lock
    HAL_FLASH_Lock();
    HAL_UART_Transmit(&huart2, (uint8_t*)"FLASH Locked\r\n", strlen("FLASH Locked\r\n"), 100);
}


void LoadDefaultsFromFlash(void) { //Funkcija nuskaityti numatytiems parametrams is atminties
    uint32_t *p = (uint32_t*)FLASH_USER_PAGE_ADDR;
    uint32_t w0 = p[0], w1 = p[1], w2 = p[2];
    if ((w0|w1|w2) == 0xFFFFFFFF) return;  // blank/not programmed

    default_KIEKIS1   =  w0        & 0xFF;
    default_KIEKIS2   = (w0 >>  8) & 0xFF;
    default_MATAVIMAS = (w0 >> 16) & 0xFFFF;

    default_OLED      =  w1        & 0xFFFF;
    default_PC        = (w1 >> 16) & 0xFFFF;

    INA219_1_ADDRESS  =  w2        & 0xFF;
    INA219_2_ADDRESS  = (w2 >>  8) & 0xFF;
}


//Sukonfiguruojame INA219 pagal tai, kiek is eiles automatiniu matavimu pacio galios matuoklio viduje norime atlikti, taip pat nustatome, kad visada matuosime ir itampa ir galia
uint8_t INA219_Config(I2C_HandleTypeDef *i2cHandle, uint8_t IRENGINIO_ADRESAS, uint8_t KIEKIS, uint8_t IRENGINIO_INDEKSAS){ 
  /*
	BIT 15  BIT 14  BIT 13  BIT 12  BIT 11  BIT 10  BIT 9   BIT 8 
	RST     X       BRNG    PG1     PG0     BADC4   BADC3   BADC2
	0	      0       1       1       1       0       0       1
	
	BIT 7   BIT 6   BIT 5   BIT 4   BIT 3   BIT 2   BIT 1   BIT 0   
	BADC1   SADC4   SADC3   SADC2   SADC1   MODE3   MODE2   MODE1
	1	      0       0       1       1       0       1       1 
	
  RST = 0, nes nenorime resetinti
  X = 0, nesvarbu koks
  BRNG = 1, nes norime bus voltage range kad butu iki 32V
  PG1-PG0 = 11, nes norime, kad PGA = /8, kadangi tada galesime tureti maksimalia 3,2 A srove
  BADC4-BADC1 = 0011, nes norime, kad 12 bitu skyra butu nuskaityta maitinimo itampos verte (si verte kintama)
  SADC4-SADC1 = 0011, nes norime, kad 12 bitu skyra butu nuskaityta tekancios sroves verte (si verte kintama)
  MODE3-MODE1 = 011, nes norime, kad trigger budu ismatuotu srove ir itampa (si verte kintama)
  */
	
	switch(KIEKIS){ //Kiek automatiniu matavimu norime atlikti
		case 1:
			config_byte[0] = 0b00111100; //bit8
			config_byte[1] = 0b01000000; //bit0
    break;
		case 2:
			config_byte[0] = 0b00111100; //bit8
      config_byte[1] = 0b11001000; //bit0
    break;
		case 4:
			config_byte[0] = 0b00111101; //bit8
      config_byte[1] = 0b01010000; //bit0
    break;
		case 8:
			config_byte[0] = 0b00111101; //bit8
      config_byte[1] = 0b11011000; //bit0
    break;
		case 16:
			config_byte[0] = 0b00111110; //bit8
      config_byte[1] = 0b01100000; //bit0
    break;
		case 32:
			config_byte[0] = 0b00111110; //bit8
      config_byte[1] = 0b11101000; //bit0
    break;
		case 64:
			config_byte[0] = 0b00111111; //bit8
      config_byte[1] = 0b01110000; //bit0
    break;
		case 128:
			config_byte[0] = 0b00111111; //bit8
      config_byte[1] = 0b11111000; //bit0
    break;
		default:
			config_byte[0] = 0b00111100; //bit8
			config_byte[1] = 0b01000000; //bit0
}
	 config_byte[1] |= 0b011; //Norime matuoti itampa ir srove
	
	 //Konfiguracijos irasomos i MV atminti, kadangi konfiuracijos irasymas i INA219 paleidzia matavimus
	 config_final[IRENGINIO_INDEKSAS*2] = config_byte[0]; //jei 0 indeksas, irasom i 0 ir 1 masyvo elementa
	 config_final[IRENGINIO_INDEKSAS*2+1] = config_byte[1]; // jei 1 indeksas, irasom i 2 ir 3 masyvo elementa

	return 0;
}


//Sukalibruojame INA219 pagal skaiciavimus, kadangi norime tureti maksimalia 3,2 A srove
uint8_t INA219_Calibrate(I2C_HandleTypeDef *i2cHandle, uint8_t IRENGINIO_ADRESAS){
	/*
	BIT 15  BIT 14  BIT 13  BIT 12  BIT 11  BIT 10  BIT 9   BIT 8   
  FS15    FS14    FS13    FS12    FS11    FS10    FS9     FS8
  0	      0       0       1       0       0       0       0
	
	BIT 7   BIT 6   BIT 5   BIT 4   BIT 3   BIT 2   BIT 1   BIT 0   
  FS7     FS6     FS5     FS4     FS3     FS2     FS1     FS0
  0	      0       0       0       0       0       0       0 
	
	BIT15-BIT0 = 00010000 00000000, nes taip apskaiciavome kalibracine verte pagal srove
	*/
	
	calib_byte[0] = 0b00010000;
	calib_byte[1] = 0b00000000;
	
	if(HAL_I2C_Mem_Write(i2cHandle, (IRENGINIO_ADRESAS << 1), CALIBRATION_REG, 1, (uint8_t*)calib_byte, 2, I2C_TIMEOUT) != HAL_OK)
		return 1;
	
	return 0;
}

//Itampos ir sroves nuskaitymas is INA219, nurodomas I2C Handler, INA219 adresas, Indeksas (kad butu paprasciau rasyti i masyva, INA219_1 indeksas yra 0, o INA219_2 indeksas yra 1
//OLED_indeksas - indeksas, i kuri itampos ir sroves masyvo elementa irasyti ismatuota verte, PC_indeksas - indeksas, i kuri itampos ir sroves masyvo elementa irasyti ismatuota verte
uint8_t INA219_Read_Bus_Voltage_And_Current(I2C_HandleTypeDef *i2cHandle, uint8_t IRENGINIO_ADRESAS, uint8_t IRENGINIO_INDEKSAS, uint8_t OLED_indeksas, uint8_t PC_indeksas){
	// Paleidziame konversija, irasydami konfiguracija i INA219 atminti
		if(HAL_I2C_Mem_Write(i2cHandle, (IRENGINIO_ADRESAS << 1), CONFIG_REG, 1, (uint8_t*)(config_final+IRENGINIO_INDEKSAS*2), 2, I2C_TIMEOUT) != HAL_OK)
			return 1;
		bus_voltage_reg[1] = 0; //Isvalome registra, kadangi pagal ji tikrinsime ar matavimas atliktas
		
    //Polling metodu tikriname ar ismatavimo flag yra 1, tada reiskiasi, kad matavimas atliktas
    while (!(bus_voltage_reg[1] & (1 << 1))){
				// Nuskaitome itampos registra, kuriame ir yra READY flag
        if (HAL_I2C_Mem_Read(i2cHandle, (IRENGINIO_ADRESAS << 1), BUS_VOLTAGE_REG, 1, bus_voltage_reg, 2, I2C_TIMEOUT) != HAL_OK)
            return 1;
		}   
					//Nuskaitome sroves registra
					if (HAL_I2C_Mem_Read(i2cHandle, (IRENGINIO_ADRESAS << 1), CURRENT_REG, 1, current_reg, 2, I2C_TIMEOUT) != HAL_OK)
							return 1;
					
					//Apskaiciuojame itampa ir pastumiame per 3 i desine, kadangi to reikalauja duomenu lapas bei yra isvalomi nereikalingi bitai, tokie kaip READY flag
					bus_voltage = (bus_voltage_reg[0] << 8) | bus_voltage_reg[1];
					bus_voltage >>= 3;
					ISMATUOTA_ITAMPA_OLED[OLED_indeksas][IRENGINIO_INDEKSAS] = bus_voltage*0.004; //Tikra itampa issaugoma OLED masyve
					ISMATUOTA_ITAMPA_PC[PC_indeksas][IRENGINIO_INDEKSAS] = ISMATUOTA_ITAMPA_OLED[OLED_indeksas][IRENGINIO_INDEKSAS]; //Tikra itampa issaugoma PC masyve

					current = (current_reg[0] << 8) | current_reg[1];
					ISMATUOTA_SROVE_OLED[OLED_indeksas][IRENGINIO_INDEKSAS] = current * 0.1; //current * 0.0001 * 1000; //Pagal calibrate verte apskaiciuojame srove ir irasome i OLED masyva
					ISMATUOTA_SROVE_PC[PC_indeksas][IRENGINIO_INDEKSAS] = ISMATUOTA_SROVE_OLED[OLED_indeksas][IRENGINIO_INDEKSAS]; //Irasome srove i PC masyva
					
					#ifdef DEBUG
					uint8_t len3;
					char buf3[30];
					len3 = snprintf(buf3, sizeof(buf3),
												 "V=(%.4f);I=(%.4f);\n",
												 ISMATUOTA_ITAMPA_OLED[OLED_indeksas][IRENGINIO_INDEKSAS],
												 ISMATUOTA_SROVE_OLED[OLED_indeksas][IRENGINIO_INDEKSAS]);
							 
					HAL_UART_Transmit(&huart2, (uint8_t*)buf3, (uint16_t)len3, HAL_MAX_DELAY);
					#endif  
    return 0;
}

uint8_t Dekodavimas(){ //Dekoduojame konfiguracini zodi is PC
	//          1111111111222222222233333333334444444444555555
  //01234567890123456789012345678901234567890123456789012345
	//KIEKIS1=016;KIEKIS2=128;MATAVIMAS=0250;OLED=1000;PC=2000
	//Toks zodis bus gaunamas is PC
	__HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
	
	if(RxBuferis[0] == 0x53 && RxBuferis[1] == 0x54 && RxBuferis[2] == 0x4F && RxBuferis[3] == 0x50){ //Patikriname ar yra "STOP" raides
		STOP = 1;
		u8g2_ClearBuffer(&u8g2); //Isvalom ekrana
    u8g2_SendBuffer(&u8g2);

		for(int i = 0; i < UART_ZINUTES_ILGIS; i++) //Isvalome buferi
			RxBuferis[i] = 0;
		
	}
	
	//Speciali komanda irasymui i flash
	//K1=001;K2=002;M=1000;O=1000;P=2000;A1=1000000;A2=1000001 
	//          1111111111222222222233333333334444444444555555
  //01234567890123456789012345678901234567890123456789012345
	
	else if(RxBuferis[1] == 0x31){ //Tikriname ar antras simbolis yra lygus 1, jei taip, pakeiciame numatytuosius parametrus
		default_KIEKIS1 = 100*(RxBuferis[3]-48)+10*(RxBuferis[4]-48)+(RxBuferis[5]-48);	
		default_KIEKIS2 = 100*(RxBuferis[10]-48)+10*(RxBuferis[11]-48)+(RxBuferis[12]-48);	
		default_MATAVIMAS = 1000*(RxBuferis[16]-48)+100*(RxBuferis[17]-48)+10*(RxBuferis[18]-48)+(RxBuferis[19]-48);
		default_OLED = 1000*(RxBuferis[23]-48)+100*(RxBuferis[24]-48)+10*(RxBuferis[25]-48)+(RxBuferis[26]-48);
		default_PC = 1000*(RxBuferis[30]-48)+100*(RxBuferis[31]-48)+10*(RxBuferis[32]-48)+(RxBuferis[33]-48);
		INA219_1_ADDRESS = 0b0000000;
		INA219_2_ADDRESS = 0b0000000;
		INA219_1_ADDRESS = 0 << 7 | (RxBuferis[38]-48) << 6 | (RxBuferis[39]-48) << 5 | (RxBuferis[40]-48) << 4 | (RxBuferis[41]-48) << 3 | (RxBuferis[42]-48) << 2 | (RxBuferis[43]-48) << 1 | (RxBuferis[44]-48);
		INA219_2_ADDRESS = 0 << 7 | (RxBuferis[49]-48) << 6 | (RxBuferis[50]-48) << 5 | (RxBuferis[51]-48) << 4 | (RxBuferis[52]-48) << 3 | (RxBuferis[53]-48) << 2 | (RxBuferis[54]-48) << 1 | (RxBuferis[55]-48);
		SaveDefaultsToFlash(); //Irasome numatytuosius parametrus i flash
		
		for(int i = 0; i < UART_ZINUTES_ILGIS; i++) //Isvalome buferi
			RxBuferis[i] = 0;
		
		
	}
	
	else if(RxBuferis[0] == 0x4B){ //Patikriname ar pradine raide yra "K"
		STOP = 0;
		//Pagal atitinkamus gauto zodzio masyvo indeksus, apskaiciuojame:
		KIEKIS1 = 100*(RxBuferis[8]-48)+10*(RxBuferis[9]-48)+(RxBuferis[10]-48);	//Kiek kartu is eiles automatiskai matuoti itampa ir srove pirmam kanalui 
		KIEKIS2 = 100*(RxBuferis[20]-48)+10*(RxBuferis[21]-48)+(RxBuferis[22]-48); //Kiek kartu is eiles automatiskai matuoti itampa ir srove antram kanalui
		MATAVIMAS = 1000*(RxBuferis[34]-48)+100*(RxBuferis[35]-48)+10*(RxBuferis[36]-48)+(RxBuferis[37]-48); //Kas kiek laiko vykdomas matavimas (ms)
		OLED = 1000*(RxBuferis[44]-48)+100*(RxBuferis[45]-48)+10*(RxBuferis[46]-48)+(RxBuferis[47]-48); //Kas kiek laiko isvesti info i OLED ekrana (ms)
		PC = 1000*(RxBuferis[52]-48)+100*(RxBuferis[53]-48)+10*(RxBuferis[54]-48)+(RxBuferis[55]-48); //Kas kiek laiko isvesti info i PC (ms)
		
		for(int i = 0; i < UART_ZINUTES_ILGIS; i++) //Isvalome buferi
			RxBuferis[i] = 0;
			
		}
	
		__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
	
	return 0;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){ //Pertrauktis, gaunant uzklausa is PC
	
    if (huart != &huart2) return;          //Ignoruoti kitus UART
		STOP = 1;
		Dekodavimas(); //Is gauto UART zodzio isskaiciuojame parametrus
		PC_laikas= PC/MATAVIMAS; 
		//apskaiciuojame, kiek matavimu telpa, kol reikia siusti info i PC. Pvz.: Jei siuntimas i PC kas 2000 ms, matavimas kas 250 ms, PC_laikas= 2000 ms/250 ms = 8
		OLED_laikas = OLED/MATAVIMAS;
		//apskaiciuojame, kiek matavimu telpa, kol reikia siusti info i OLED. Pvz.: Jei siuntimas i OLED kas 2000 ms, matavimas kas 250 ms, OLED_laikas = 2000 ms/250 ms = 8
		MATAVIMO_laikas = MATAVIMAS/TIMER_INTERRUPT_TIME;
	  //apskaiciuojame, kiek laikmacio pertraukciu ivyksta, kol reikia matavimo. Pvz.: Jei matuojame kas 500 ms, o laikmatis sudirba kas 250 ms, MATAVIMO_laikas = 2

		INA219_Config(&hi2c2,INA219_1_ADDRESS, KIEKIS1, 0); //Sukonfiguruojame pirma INA219
		INA219_Config(&hi2c2,INA219_2_ADDRESS, KIEKIS2, 1); //Sukonfiguruojame antra INA219
	
    HAL_UART_Receive_IT(&huart2, RxBuferis, UART_ZINUTES_ILGIS); //Laukiame kitos UART pertraukties
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) //Mygtuko pertrauktis
{
    if (GPIO_Pin == GPIO_PIN_13) {
    	if(STOP == 1){ //jeigu viskas isjungta, mygtuko paspaudimo metu irasomos default reiksmes
				HAL_NVIC_DisableIRQ(EXTI4_15_IRQn); //Atjungiam petrauktis debouncinimui, matavimo funkcijos pradzioje vel leidziamos pertrauktys
				
				STOP = 0;
				PC = default_PC;
				OLED = default_OLED;
				MATAVIMAS = default_MATAVIMAS;
				KIEKIS1 = default_KIEKIS1;
				KIEKIS2 = default_KIEKIS2;
				PC_laikas= PC/MATAVIMAS; 
				//apskaiciuojame, kiek matavimu telpa, kol reikia siusti info i PC. Pvz.: Jei siuntimas i PC kas 2000 ms, matavimas kas 250 ms, PC_laikas= 2000 ms/250 ms = 8
				OLED_laikas = OLED/MATAVIMAS;
				//apskaiciuojame, kiek matavimu telpa, kol reikia siusti info i OLED. Pvz.: Jei siuntimas i OLED kas 2000 ms, matavimas kas 250 ms, OLED_laikas = 2000 ms/250 ms = 8
				MATAVIMO_laikas = MATAVIMAS/TIMER_INTERRUPT_TIME;
				//apskaiciuojame, kiek laikmacio pertraukciu ivyksta, kol reikia matavimo. Pvz.: Jei matuojame kas 500 ms, o laikmatis sudirba kas 250 ms, MATAVIMO_laikas = 2
				INA219_Config(&hi2c2,INA219_1_ADDRESS, KIEKIS1, 0); //Sukonfiguruojame pirma INA219
				INA219_Config(&hi2c2,INA219_2_ADDRESS, KIEKIS2, 1); //Sukonfiguruojame antra INA219
			}
			else{
				STOP = 1;
				HAL_UART_Transmit(&huart2, (uint8_t *)"Mygtukas_STOP\n", 14, HAL_MAX_DELAY); //Issiunciam pranesima apie pabaiga ir poreiki stabdyti Python programa
				u8g2_ClearBuffer(&u8g2); //Isvalom ekrana
				u8g2_SendBuffer(&u8g2);
			}
    }
}

//Laikmacio pertrauktis, is esmes pagrindine programos dalis
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  //Tikrinti ar tai butent TIM2 timeris
  if (htim->Instance == TIM2)
  {
			if(RxBuferis[0] == 0x53){ //Kartais atsirasdavo S raide UART buferyje ir STOP nebudavo pamatytas
				RxBuferis[0] = 0;
				STOP = 1;
				HAL_UART_Receive_IT(&huart2, RxBuferis, UART_ZINUTES_ILGIS); //Laukiame kitos UART pertraukties
				u8g2_ClearBuffer(&u8g2); //Isvalom ekrana
				u8g2_SendBuffer(&u8g2);
			}
			
			PC_laikas= PC/MATAVIMAS; 
			OLED_laikas = OLED/MATAVIMAS;
			MATAVIMO_laikas = MATAVIMAS/TIMER_INTERRUPT_TIME;
			
		//Tikriname ar galima vykdyti programa, ar bent viena konfiguracija teisinga (PC) ir ar laikas vykdyti matavima, 
		// pagal tai ar praejo pakankamai laikmacio petraukciu. Tarkime, matavimai atliekami kas 1000 ms, laikmacio pertrauktis kas 250 ms,
		// tada turi praeiti 4 pertrauktys, kad butu vykdomas matavimas
    if (!STOP && PC != 0 && MATAVIMO_indeksas >= MATAVIMO_laikas)
    {
		  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn); //Vel leidziame mygtuko paspaudimus
			MATAVIMO_indeksas = 1;
			
			#ifdef DEBUG
			HAL_UART_Transmit(&huart2, (uint8_t *)"Matavimas_start\n", 16, HAL_MAX_DELAY);
			#endif  
			
			//Itampos ir sroves nuskaitymas is INA219, nurodomas I2C Handler, INA219 adresas, Indeksas (kad butu paprasciau rasyti i masyva, INA219_1 indeksas yra 0, o INA219_2 indeksas yra 1
      //OLED_indeksas - indeksas, i kuri itampos ir sroves masyvo elementa irasyti ismatuota verte, PC_indeksas - indeksas, i kuri itampos ir sroves masyvo elementa irasyti ismatuota verte
      INA219_Read_Bus_Voltage_And_Current(&hi2c2,INA219_1_ADDRESS, 0, OLED_indeksas, PC_indeksas);
			INA219_Read_Bus_Voltage_And_Current(&hi2c2,INA219_2_ADDRESS, 1, OLED_indeksas, PC_indeksas);
			
			#ifdef DEBUG
			HAL_UART_Transmit(&huart2, (uint8_t *)"Matavimas_end\n", 14, HAL_MAX_DELAY);
			#endif
			
			PC_indeksas++;
			
			if(PC_indeksas >= (PC_laikas)){ //Tikrinama ar praejo tinkamas kiekis laikmacio petraukciu
				
				#ifdef DEBUG
				HAL_UART_Transmit(&huart2, (uint8_t *)"Pc_start\n", 9, HAL_MAX_DELAY);
				#endif
				
				//Suskaiciuojamos ismatuotu verciu sumos
				for(int i = 0; i<(PC_laikas); i++){
					ITAMPOS_VIDURKIS_PC[0]+=ISMATUOTA_ITAMPA_PC[i][0];
					ITAMPOS_VIDURKIS_PC[1]+=ISMATUOTA_ITAMPA_PC[i][1];
					SROVES_VIDURKIS_PC[0]+=ISMATUOTA_SROVE_PC[i][0];
					SROVES_VIDURKIS_PC[1]+=ISMATUOTA_SROVE_PC[i][1];
				}
				
				//Suskaiciuojami vidurkiai
				if(ITAMPOS_VIDURKIS_PC[0] != 0)
					ITAMPOS_VIDURKIS_PC[0]/=(PC_laikas);
				if(ITAMPOS_VIDURKIS_PC[1] != 0)
					ITAMPOS_VIDURKIS_PC[1]/=(PC_laikas);
				if(SROVES_VIDURKIS_PC[0] != 0)
					SROVES_VIDURKIS_PC[0]/=(PC_laikas);
				if(SROVES_VIDURKIS_PC[1] != 0)
					SROVES_VIDURKIS_PC[1]/=(PC_laikas);
				
				//Sukuriamas vienas ilgas zodis, kuri siusime per UART i PC
				len = snprintf(buf, sizeof(buf),
												 "V1=(%.4f);V2=(%.4f);"
												 "I1=(%.4f);I2=(%.4f)\n",
												 ITAMPOS_VIDURKIS_PC[0],
												 ITAMPOS_VIDURKIS_PC[1],
												 SROVES_VIDURKIS_PC[0],
												 SROVES_VIDURKIS_PC[1]);
				
				//Siunciame zodi i PC
				HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)len, HAL_MAX_DELAY);
				
				PC_indeksas = 0;
				ITAMPOS_VIDURKIS_PC[0]=0;
				ITAMPOS_VIDURKIS_PC[1]=0;
				SROVES_VIDURKIS_PC[0]=0;
				SROVES_VIDURKIS_PC[1]=0;
				
				#ifdef DEBUG
				HAL_UART_Transmit(&huart2, (uint8_t *)"PC_end\n", 7, HAL_MAX_DELAY);
				#endif
			}

			OLED_indeksas++;
			
			if(OLED_indeksas >= (OLED_laikas)){ //Tikrinama ar praejo tinkamas kiekis laikmacio petraukciu
				#ifdef DEBUG
				HAL_UART_Transmit(&huart2, (uint8_t *)"Oled_start\n", 11, HAL_MAX_DELAY);
				#endif
				//Suskaiciuojamos ismatuotu verciu sumos
				for(int i = 0; i<(OLED_laikas); i++){ 
					ITAMPOS_VIDURKIS_OLED[0]+=ISMATUOTA_ITAMPA_OLED[i][0];
					ITAMPOS_VIDURKIS_OLED[1]+=ISMATUOTA_ITAMPA_OLED[i][1];
					SROVES_VIDURKIS_OLED[0]+=ISMATUOTA_SROVE_OLED[i][0];
					SROVES_VIDURKIS_OLED[1]+=ISMATUOTA_SROVE_OLED[i][1];
				}
				//Suskaiciuojami vidurkiai
				if(ITAMPOS_VIDURKIS_OLED[0] != 0)
					ITAMPOS_VIDURKIS_OLED[0]/=(OLED_laikas);
				if(ITAMPOS_VIDURKIS_OLED[1] != 0)
					ITAMPOS_VIDURKIS_OLED[1]/=(OLED_laikas);
				if(SROVES_VIDURKIS_OLED[0] != 0)
					SROVES_VIDURKIS_OLED[0]/=(OLED_laikas);
				if(SROVES_VIDURKIS_OLED[1] != 0)
					SROVES_VIDURKIS_OLED[1]/=(OLED_laikas);
				
				char oled_buf[100];
				u8g2_ClearBuffer(&u8g2);
				//Vertes siunciamos i OLED ekrana
				snprintf(oled_buf, sizeof(oled_buf), "V1=%.4f V   ", ITAMPOS_VIDURKIS_OLED[0]);
				u8g2_DrawStr(&u8g2, 10, 10, oled_buf);
				snprintf(oled_buf, sizeof(oled_buf), "V2=%.4f V   ", ITAMPOS_VIDURKIS_OLED[1]);
				u8g2_DrawStr(&u8g2, 10, 20, oled_buf);
				//Kadangi sroves gali buti neigiamos, kad nebutu sokinejimo atsiradus minusui, pridedamas tarpas
				if(SROVES_VIDURKIS_OLED[0] >= 0){
					snprintf(oled_buf, sizeof(oled_buf), "I1= %.4f mA   ", SROVES_VIDURKIS_OLED[0]); //Srove
					u8g2_DrawStr(&u8g2, 10, 30, oled_buf);
					
					snprintf(oled_buf, sizeof(oled_buf), "P1= %.4f mW   ", SROVES_VIDURKIS_OLED[0]*ITAMPOS_VIDURKIS_OLED[0]); //Galia
					u8g2_DrawStr(&u8g2, 10, 50, oled_buf);
				}
				else{
					snprintf(oled_buf, sizeof(oled_buf), "I1=%.4f mA   ", SROVES_VIDURKIS_OLED[0]); //Srove
					u8g2_DrawStr(&u8g2, 10, 30, oled_buf);
					
					snprintf(oled_buf, sizeof(oled_buf), "P1=%.4f mW   ", SROVES_VIDURKIS_OLED[0]*ITAMPOS_VIDURKIS_OLED[0]); //Galia
					u8g2_DrawStr(&u8g2, 10, 50, oled_buf);
				}
				//Kadangi sroves gali buti neigiamos, kad nebutu sokinejimo atsiradus minusui, pridedamas tarpas
				if(SROVES_VIDURKIS_OLED[1] >= 0){
					snprintf(oled_buf, sizeof(oled_buf), "I2= %.4f mA   ", SROVES_VIDURKIS_OLED[1]); //Srove
					u8g2_DrawStr(&u8g2, 10, 40, oled_buf);
					
					snprintf(oled_buf, sizeof(oled_buf), "P2= %.4f mW   ", SROVES_VIDURKIS_OLED[1]*ITAMPOS_VIDURKIS_OLED[1]); //Galia
					u8g2_DrawStr(&u8g2, 10, 60, oled_buf);
				}
				else{
					snprintf(oled_buf, sizeof(oled_buf), "I2=%.4f mA   ", SROVES_VIDURKIS_OLED[1]); //Srove
					u8g2_DrawStr(&u8g2, 10, 40, oled_buf);
					
					snprintf(oled_buf, sizeof(oled_buf), "P2=%.4f mW   ", SROVES_VIDURKIS_OLED[1]*ITAMPOS_VIDURKIS_OLED[1]); //Galia
					u8g2_DrawStr(&u8g2, 10, 60, oled_buf);
				}
				
				#ifdef DEBUG
					uint8_t len3;
					char buf3[80];
					len3 = snprintf(buf3, sizeof(buf3),
												 "V1=(%.4f);V2=(%.4f);I1=(%.4f);I2=(%.4f);\n",
												 ITAMPOS_VIDURKIS_OLED[0],
												 ITAMPOS_VIDURKIS_OLED[1], SROVES_VIDURKIS_OLED[0], SROVES_VIDURKIS_OLED[1]);
							 
					HAL_UART_Transmit(&huart2, (uint8_t*)buf3, (uint16_t)len3, HAL_MAX_DELAY);
				#endif
				
				//Nusiunciama galutine informacija i OLED
				u8g2_SendBuffer(&u8g2);
				OLED_indeksas = 0;
				ITAMPOS_VIDURKIS_OLED[0]=0;
				ITAMPOS_VIDURKIS_OLED[1]=0;
				SROVES_VIDURKIS_OLED[0]=0;
				SROVES_VIDURKIS_OLED[1]=0;
				
				#ifdef DEBUG
				HAL_UART_Transmit(&huart2, (uint8_t *)"Oled_end\n", 9, HAL_MAX_DELAY);
				#endif
			}
			
    }
		else
			MATAVIMO_indeksas++; 
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	LoadDefaultsFromFlash(); //Nuskaitome numatytasias reiksmes is flash atminties
	PC = default_PC;
	OLED = default_OLED;
	MATAVIMAS = default_MATAVIMAS;
	KIEKIS1 = default_KIEKIS1;
	KIEKIS2 = default_KIEKIS2;
	PC_laikas= PC/MATAVIMAS; 
	//apskaiciuojame, kiek matavimu telpa, kol reikia siusti info i PC. Pvz.: Jei siuntimas i PC kas 2000 ms, matavimas kas 250 ms, PC_laikas= 2000 ms/250 ms = 8
	OLED_laikas = OLED/MATAVIMAS;
	//apskaiciuojame, kiek matavimu telpa, kol reikia siusti info i OLED. Pvz.: Jei siuntimas i OLED kas 2000 ms, matavimas kas 250 ms, OLED_laikas = 2000 ms/250 ms = 8
	MATAVIMO_laikas = MATAVIMAS/TIMER_INTERRUPT_TIME;
	//apskaiciuojame, kiek laikmacio pertraukciu ivyksta, kol reikia matavimo. Pvz.: Jei matuojame kas 500 ms, o laikmatis sudirba kas 250 ms, MATAVIMO_laikas = 2
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Base_Start_IT(&htim2);

	HAL_UART_Receive_IT(&huart2, RxBuferis, UART_ZINUTES_ILGIS); //Nustatome, kad lauktu UART pertraukties is PC
	
	INA219_Calibrate(&hi2c2, INA219_1_ADDRESS); //Sukalibruojame pirma INA219
	INA219_Calibrate(&hi2c2, INA219_2_ADDRESS); //Sukalibruojame antra INA219
	
	//Kodas skirtas inicializuoti SH1106 OLED
	u8g2_Setup_sh1106_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_stm32_hw_i2c, u8x8_stm32_gpio_and_delay);
	u8g2_InitDisplay(&u8g2); 
	u8g2_SetPowerSave(&u8g2, 0);
	u8g2_ClearBuffer(&u8g2);
	u8g2_SendBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
	
	INA219_Config(&hi2c2,INA219_1_ADDRESS, KIEKIS1, 0); //Sukonfiguruojame pirma INA219
	INA219_Config(&hi2c2,INA219_2_ADDRESS, KIEKIS2, 1); //Sukonfiguruojame antra INA219

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
			
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00503D58;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 15999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 249;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
