#include <neorv32.h>
#include <stdint.h>

#define PIN_ALARM_LED 1
#define PIN_AC_LED    2  
#define PIN_LIGHT_LED 3  
#define BUFFER_SIZE   16

const uint8_t aes_key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

// --- Funções Auxiliares ---
static inline uint32_t bswap32(uint32_t x) {
    return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000);
}

static inline uint32_t aes32dsmi(uint32_t rs1, uint32_t rs2, int bs) {
    uint32_t rd;
    if (bs == 0)      asm volatile ("aes32dsmi %0, %1, %2, 0" : "=r"(rd) : "r"(rs1), "r"(rs2));
    else if (bs == 1) asm volatile ("aes32dsmi %0, %1, %2, 1" : "=r"(rd) : "r"(rs1), "r"(rs2));
    else if (bs == 2) asm volatile ("aes32dsmi %0, %1, %2, 2" : "=r"(rd) : "r"(rs1), "r"(rs2));
    else              asm volatile ("aes32dsmi %0, %1, %2, 3" : "=r"(rd) : "r"(rs1), "r"(rs2));
    return rd;
}

static inline uint32_t aes32dsi(uint32_t rs1, uint32_t rs2, int bs) {
    uint32_t rd;
    if (bs == 0)      asm volatile ("aes32dsi %0, %1, %2, 0" : "=r"(rd) : "r"(rs1), "r"(rs2));
    else if (bs == 1) asm volatile ("aes32dsi %0, %1, %2, 1" : "=r"(rd) : "r"(rs1), "r"(rs2));
    else if (bs == 2) asm volatile ("aes32dsi %0, %1, %2, 2" : "=r"(rd) : "r"(rs1), "r"(rs2));
    else              asm volatile ("aes32dsi %0, %1, %2, 3" : "=r"(rd) : "r"(rs1), "r"(rs2));
    return rd;
}

static inline uint8_t xtime(uint8_t x) { return (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00); }
static inline uint8_t mul_gf(uint8_t x, uint8_t c) {
    uint8_t x2 = xtime(x), x4 = xtime(x2), x8 = xtime(x4), res = 0;
    if (c & 1) res ^= x; if (c & 2) res ^= x2; if (c & 4) res ^= x4; if (c & 8) res ^= x8;
    return res;
}

uint32_t aes32imc_sw(uint32_t w) {
    uint8_t b[4] = {w, w >> 8, w >> 16, w >> 24}, r[4];
    r[0] = mul_gf(b[0], 0x0e) ^ mul_gf(b[1], 0x0b) ^ mul_gf(b[2], 0x0d) ^ mul_gf(b[3], 0x09);
    r[1] = mul_gf(b[0], 0x09) ^ mul_gf(b[1], 0x0e) ^ mul_gf(b[2], 0x0b) ^ mul_gf(b[3], 0x0d);
    r[2] = mul_gf(b[0], 0x0d) ^ mul_gf(b[1], 0x09) ^ mul_gf(b[2], 0x0e) ^ mul_gf(b[3], 0x0b);
    r[3] = mul_gf(b[0], 0x0b) ^ mul_gf(b[1], 0x0d) ^ mul_gf(b[2], 0x09) ^ mul_gf(b[3], 0x0e);
    return (uint32_t)r[0] | (r[1] << 8) | (r[2] << 16) | (r[3] << 24);
}

void aes128_key_expansion_dec(const uint8_t *key, uint32_t *rk) {
    static const uint8_t sbox[256] = {
        0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
        0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
        0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
        0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
        0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
        0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
        0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
        0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
        0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
        0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
        0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
        0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
        0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
        0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
        0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
        0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
    };
    for (int i = 0; i < 4; i++) rk[i] = ((uint32_t*)key)[i];
    for (int i = 4; i < 44; i++) {
        uint32_t t = rk[i-1];
        if (i % 4 == 0) {
            uint32_t rot = (t >> 8) | (t << 24);
            t = (uint32_t)sbox[rot & 0xff] | (sbox[(rot >> 8) & 0xff] << 8) | (sbox[(rot >> 16) & 0xff] << 16) | (sbox[rot >> 24] << 24);
            static const uint32_t rcon[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36};
            t ^= rcon[i/4 - 1];
        }
        rk[i] = rk[i-4] ^ t;
    }
    for (int i = 4; i < 40; i++) rk[i] = aes32imc_sw(rk[i]);
}

void aes128_decrypt_hw(uint8_t *buffer, uint32_t *rk) {
    uint32_t s[4];
    // Carrega os bytes do buffer para as palavras de 32 bits (Little-Endian)
    for(int i=0; i<4; i++) {
        s[i] = ((uint32_t)buffer[i*4+0]) | ((uint32_t)buffer[i*4+1] << 8) | 
               ((uint32_t)buffer[i*4+2] << 16) | ((uint32_t)buffer[i*4+3] << 24);
    }

    s[0] ^= rk[40]; s[1] ^= rk[41]; s[2] ^= rk[42]; s[3] ^= rk[43];

    for (int r = 9; r >= 1; r--) {
        uint32_t t0 = rk[4*r+0] ^ aes32dsmi(0, s[0], 0) ^ aes32dsmi(0, s[3], 1) ^ aes32dsmi(0, s[2], 2) ^ aes32dsmi(0, s[1], 3);
        uint32_t t1 = rk[4*r+1] ^ aes32dsmi(0, s[1], 0) ^ aes32dsmi(0, s[0], 1) ^ aes32dsmi(0, s[3], 2) ^ aes32dsmi(0, s[2], 3);
        uint32_t t2 = rk[4*r+2] ^ aes32dsmi(0, s[2], 0) ^ aes32dsmi(0, s[1], 1) ^ aes32dsmi(0, s[0], 2) ^ aes32dsmi(0, s[3], 3);
        uint32_t t3 = rk[4*r+3] ^ aes32dsmi(0, s[3], 0) ^ aes32dsmi(0, s[2], 1) ^ aes32dsmi(0, s[1], 2) ^ aes32dsmi(0, s[0], 3);
        s[0]=t0; s[1]=t1; s[2]=t2; s[3]=t3;
    }

    uint32_t final[4];
    final[0] = rk[0] ^ aes32dsi(0, s[0], 0) ^ aes32dsi(0, s[3], 1) ^ aes32dsi(0, s[2], 2) ^ aes32dsi(0, s[1], 3);
    final[1] = rk[1] ^ aes32dsi(0, s[1], 0) ^ aes32dsi(0, s[0], 1) ^ aes32dsi(0, s[3], 2) ^ aes32dsi(0, s[2], 3);
    final[2] = rk[2] ^ aes32dsi(0, s[2], 0) ^ aes32dsi(0, s[1], 1) ^ aes32dsi(0, s[0], 2) ^ aes32dsi(0, s[3], 3);
    final[3] = rk[3] ^ aes32dsi(0, s[3], 0) ^ aes32dsi(0, s[2], 1) ^ aes32dsi(0, s[1], 2) ^ aes32dsi(0, s[0], 3);

    // Inverte e salva de volta no buffer
    ((uint32_t*)buffer)[0] = bswap32(final[0]);
    ((uint32_t*)buffer)[1] = bswap32(final[1]);
    ((uint32_t*)buffer)[2] = bswap32(final[2]);
    ((uint32_t*)buffer)[3] = bswap32(final[3]);
}

int main() {
    neorv32_rte_setup();
    neorv32_uart0_setup(19200, 0); 

    neorv32_gpio_pin_set(PIN_ALARM_LED, 0);
    neorv32_gpio_pin_set(PIN_AC_LED, 0);
    neorv32_gpio_pin_set(PIN_LIGHT_LED, 0);

    uint32_t round_keys[44];
    aes128_key_expansion_dec(aes_key, round_keys);

    uint8_t buffer[BUFFER_SIZE]; 
    int index = 0;                

    neorv32_uart0_puts("Sistema Pronto.\n");

    while (1) {
        if (neorv32_uart0_available()) {
            // LER BYTE A BYTE E COLOCAR NO ARRAY uint8_t
            buffer[index++] = (uint8_t)neorv32_uart_getc(NEORV32_UART0);

            if (index >= BUFFER_SIZE) {
                neorv32_uart0_puts("\n--- Novo Bloco ---\n");
                
                // Print do Cifrado (HEX Byte a Byte)
                neorv32_uart0_puts("CIFRADO: ");
                for(int i=0; i<16; i++) neorv32_uart0_printf("%2x ", (uint32_t)(buffer[i] & 0xFF));
                neorv32_uart0_puts("\n");

                aes128_decrypt_hw(buffer, round_keys);

                // Print do Descriptografado
                neorv32_uart0_puts("PLAIN (BE): ");
                for(int i=0; i<4; i++) neorv32_uart0_printf("%x ", ((uint32_t*)buffer)[i]);
                neorv32_uart0_puts("\n");
				
				//neorv32_uart0_printf("PLAIN (BE): %x %x %x %x\n", ((uint32_t*)cipher)[0], ((uint32_t*)cipher)[1], ((uint32_t*)cipher)[2], ((uint32_t*)cipher)[3]);


                uint8_t dispositivo = buffer[3]; // Pega o 0xA1
				uint8_t estado      = buffer[2]; // Pega o 0x01
                neorv32_uart0_printf("ID: %x, STATUS: %x\n", dispositivo, estado);
				
				//neorv32_gpio_pin_set(PIN_ALARM_LED, 1);

                if (dispositivo == 0xA1) neorv32_gpio_pin_set(PIN_ALARM_LED, (int)estado);
                if (dispositivo == 0xAC) neorv32_gpio_pin_set(PIN_AC_LED, (int)estado);
                if (dispositivo == 0xBB) neorv32_gpio_pin_set(PIN_LIGHT_LED, (int)estado);

                index = 0; 
            }
        }
    }
    return 0;
}