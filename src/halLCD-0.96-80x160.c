#include "halPaint.h"
#include "board.h"
    
#if (BOARD_ESP8266DIOTEST_ALL || BOARD_nRF52832_TEST || BOARD_ESP32SWATCH)




struct hal_screen_s_t {
	U8 orientation;
    //COLOR color;
} hal_screen_s;


#define ST7735_SLEEPOUTBOOSTERON				0x11 //Sleep out & booster on
#define ST7735_DISPLAYINVON						0x21 //Display inversion on
#define ST7735_DISPLAYON						0x29 //Display inversion on


#if BOARD_ESP8266DIOTEST_ALL

#if LCD_SPI_SOFTWARE
#else
#include "halSPI.h"
#endif

#define HAL_PAINT_DATA					13
#define HAL_PAINT_CLK					14
#define HAL_PAINT_RS					9  //0 - command, 1 - data
#define HAL_PAINT_CS					2
#define HAL_PAINT_RES					0
#define HAL_PAINT_LED					15

#define HAL_PAINT_DC_L					GPIO_OUTPUT_SET(HAL_PAINT_RS, LOW) //GPIO_REG_WRITE (GPIO_OUT_W1TC_ADDRESS, 1 << 12) //
#define HAL_PAINT_DC_H					GPIO_OUTPUT_SET(HAL_PAINT_RS, HIGH) //GPIO_REG_WRITE (GPIO_OUT_W1TS_ADDRESS, 1 << 12) //
#define HAL_PAINT_CS_L					GPIO_OUTPUT_SET(HAL_PAINT_CS, LOW)
#define HAL_PAINT_CS_H					GPIO_OUTPUT_SET(HAL_PAINT_CS, HIGH)
#define HAL_PAINT_LED_L					GPIO_OUTPUT_SET(HAL_PAINT_LED, LOW)
#define HAL_PAINT_LED_H					GPIO_OUTPUT_SET(HAL_PAINT_LED, HIGH)
#define HAL_PAINT_RES_L					GPIO_OUTPUT_SET(HAL_PAINT_RES, LOW)
#define HAL_PAINT_RES_H					GPIO_OUTPUT_SET(HAL_PAINT_RES, HIGH)

#define HAL_PAINT_DELAY      			1

#endif


#if BOARD_nRF52832_TEST

#endif



#if BOARD_ESP32SWATCH
spi_device_handle_t spi;

//Send a command to the LCD. Uses spi_device_transmit, which waits until the transfer is complete.


//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    //gpio_set_level(GPIO_NUM_27, dc);
}



void hal_spi_send_8bit (U8 spi_no, U8 dout_data)
{
#if LCD_SPI_SOFTWARE
	S8 i;

	for (i = 0; i < 8; i++)
	{
		if (dout_data & 0x80)
		{
			HAL_PAINT_DATA_H;
		}
		else
		{
			HAL_PAINT_DATA_L;
		}
		dout_data = dout_data << 1;
		__NOP();///os_delay_us (HAL_PAINT_DELAY);
		HAL_PAINT_CLK_H;
		__NOP();//os_delay_us (HAL_PAINT_DELAY);
		HAL_PAINT_CLK_L;
		__NOP();//os_delay_us (HAL_PAINT_DELAY);
	}
	__NOP();//os_delay_us (HAL_PAINT_DELAY);
#else
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8;                     //Command is 8 bits
    t.tx_buffer = &dout_data;               //The data is the cmd itself
    t.user = (void*)0;                //D/C needs to be set to 0
    //HAL_PAINT_DC_L;
    ret = spi_device_transmit(spi, &t);  //Transmit!
    assert(ret == ESP_OK);            //Should have had no issues.
#endif
}


void hal_spi_send_512bit (U8 spi_no, U16 *ptr16)
{
    U8 *ptr8 = (U8 *)ptr16;
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8*64;                     //Command is 8 bits
    t.tx_buffer = ptr8;               //The data is the cmd itself
    t.user = (void*)0;                //D/C needs to be set to 0
    //HAL_PAINT_DC_L;
    ret = spi_device_transmit(spi, &t);  //Transmit!
    assert(ret == ESP_OK);            //Should have had no issues.
}


void hal_spi_send_all (U8 spi_no, U16 *ptr16)
{
    U8 *ptr8 = (U8 *)ptr16;
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8*160*80*2;                     //Command is 8 bits
    t.tx_buffer = ptr8;               //The data is the cmd itself
    t.user = (void*)0;                //D/C needs to be set to 0
    //HAL_PAINT_DC_L;
    ret = spi_device_transmit(spi, &t);  //Transmit!
    assert(ret == ESP_OK);            //Should have had no issues.
}

#endif


#define HSPI                            1
extern void hal_spi_send_8bit (U8 spi_no, U8 dout_data);
extern void hal_spi_send_512bit (U8 spi_no, U16 *ptr);

COLOR screen_buf [SCREEN_VIRTUAL_W * SCREEN_VIRTUAL_H + 32];


void hal_paint_send_8bit (U8 data)
{
	hal_spi_send_8bit (HSPI, data);
}


void hal_paint_send_command (U8 command)
{
    HAL_PAINT_DC_L;
    hal_paint_send_8bit (command);
}


void CODE hal_paint_deinit (U8 mode)
{
	HAL_PAINT_CS_L;
	hal_paint_send_command (0x28);
	_delay_ms (2);
	hal_paint_send_command (0x10);
	HAL_PAINT_CS_H;
	HAL_PAINT_LED_L;
}


void CODE hal_paint_init (U8 mode)
{
#if BOARD_ESP8266DIOTEST_ALL
        // Configure pin as a GPIO
    #if LCD_SPI_SOFTWARE
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2); //CS
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); //RS
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); //CLK
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14); //DAT
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); //LED
        HAL_PAINT_DATA_L;
        HAL_PAINT_CLK_L;
    #else

        halSPI_spi_init (HSPI,
                0,
                1, 2, //80MHz / 1 / 2 = 40MHz
                SPI_BYTE_ORDER_HIGH_TO_LOW,
                SPI_BYTE_ORDER_HIGH_TO_LOW);
        //hal_spi_send_64byte (HSPI, screen_buf); //test
        //init
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO0); //RES
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9); //MOTOR
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2); //CS
        PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); //LED
        
    #endif
#endif
#if BOARD_ESP32SWATCH
	dbgprintf ("Start init HSPI\r\n");
	HAL_PAINT_CS_H;
	HAL_PAINT_DC_L;
	gpio_set_direction(HAL_PAINT_DC, GPIO_MODE_OUTPUT);
	gpio_set_direction(HAL_PAINT_RES, GPIO_MODE_OUTPUT);
	gpio_set_direction(HAL_PAINT_LED, GPIO_MODE_OUTPUT);
	gpio_set_direction(HAL_PAINT_CS, GPIO_MODE_OUTPUT);

	esp_err_t ret;
	//spi_device_handle_t spi;
	spi_bus_config_t buscfg={
		.miso_io_num = -1,//19,//12 not allowed!!!!!!!!!
		.mosi_io_num = HAL_PAINT_DATA,
		.sclk_io_num = HAL_PAINT_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 160*80*2+32 //PAINT_BUF_SIZE*2+8 //320*2+8
	};

	spi_device_interface_config_t devcfg={
		.clock_speed_hz = 32000000,          //Clock out at 40 MHz
		.mode = 0,                                //SPI mode 0
		.spics_io_num = -1,               //CS pin
		.queue_size = 7,                          //We want to be able to queue 7 transactions at a time
		.pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
	};


	//Initialize the SPI bus
	ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	ESP_ERROR_CHECK(ret);

	//Attach the LCD to the SPI bus
	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);

#endif

    HAL_PAINT_LED_L;
	HAL_PAINT_CS_H;
	HAL_PAINT_DC_L;
	//start
	HAL_PAINT_RES_L;
	_delay_ms (2);
	HAL_PAINT_RES_H;
	HAL_PAINT_LED_H;
	_delay_ms (1);
	HAL_PAINT_CS_L;
	_delay_ms (2);
	hal_paint_send_command ( ST7735_SLEEPOUTBOOSTERON );
	_delay_ms (2);
	hal_paint_send_command ( ST7735_DISPLAYINVON );
	hal_paint_send_command ( 0xB1 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x05 );
	hal_paint_send_8bit ( 0x3A );
	hal_paint_send_8bit ( 0x3A );

	hal_paint_send_command ( 0xB2 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x05 );
	hal_paint_send_8bit ( 0x3A );
	hal_paint_send_8bit ( 0x3A );

	hal_paint_send_command ( 0xB3 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x05 );
	hal_paint_send_8bit ( 0x3A );
	hal_paint_send_8bit ( 0x3A );
	hal_paint_send_8bit ( 0x05 );
	hal_paint_send_8bit ( 0x3A );
	hal_paint_send_8bit ( 0x3A );

	hal_paint_send_command ( 0xB4 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x03 );

	hal_paint_send_command ( 0xC0 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x62 );
	hal_paint_send_8bit ( 0x02 );
	hal_paint_send_8bit ( 0x04 );

	hal_paint_send_command ( 0xC1 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0xC0 );

	hal_paint_send_command ( 0xC2 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x0D );
	hal_paint_send_8bit ( 0x00 );

	hal_paint_send_command ( 0xC3 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x8D );
	hal_paint_send_8bit ( 0x6A );

	hal_paint_send_command ( 0xC4 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x8D );
	hal_paint_send_8bit ( 0xEE );

	hal_paint_send_command ( 0xC5 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x0E );

	hal_paint_send_command ( 0xE0 );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x10 );
	hal_paint_send_8bit ( 0x0E );
	hal_paint_send_8bit ( 0x02 );
	hal_paint_send_8bit ( 0x03 );
	hal_paint_send_8bit ( 0x0E );
	hal_paint_send_8bit ( 0x07 );
	hal_paint_send_8bit ( 0x02 );
	hal_paint_send_8bit ( 0x07 );
	hal_paint_send_8bit ( 0x0A );
	hal_paint_send_8bit ( 0x12 );
	hal_paint_send_8bit ( 0x27 );
	hal_paint_send_8bit ( 0x37 );
	hal_paint_send_8bit ( 0x00 );
	hal_paint_send_8bit ( 0x0D );
	hal_paint_send_8bit ( 0x0E );
	hal_paint_send_8bit ( 0x10 );

	hal_paint_send_command ( 0xE1 ); //
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x10 );
	hal_paint_send_8bit ( 0x0E );
	hal_paint_send_8bit ( 0x03 );
	hal_paint_send_8bit ( 0x03 );
	hal_paint_send_8bit ( 0x0F );
	hal_paint_send_8bit ( 0x06 );
	hal_paint_send_8bit ( 0x02 );
	hal_paint_send_8bit ( 0x08 );
	hal_paint_send_8bit ( 0x0A );
	hal_paint_send_8bit ( 0x13 );
	hal_paint_send_8bit ( 0x26 );
	hal_paint_send_8bit ( 0x36 );
	hal_paint_send_8bit ( 0x00 );
	hal_paint_send_8bit ( 0x0D );
	hal_paint_send_8bit ( 0x0E );
	hal_paint_send_8bit ( 0x10 );

	hal_paint_send_command ( 0x3A ); //Interface Pixel Format
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( 0x05 );

	hal_paint_send_command ( ST7735_DISPLAYON );
	HAL_PAINT_CS_H;

	hal_paint_set_orientation ( SCREEN_ORIENTATION_0 );
}


void hal_paint_set_window (COORD x1, COORD x2, COORD y1, COORD y2)
{
	switch (hal_screen_s.orientation)
	{
	case SCREEN_ORIENTATION_0:
	case SCREEN_ORIENTATION_180:
		x1 = x1 + 26;
		x2 = x2 + 26;
		y1 = y1 + 1;
		y2 = y2 + 1;
		break;
	case SCREEN_ORIENTATION_90:
	case SCREEN_ORIENTATION_270:
		x1 = x1 + 1;
		x2 = x2 + 1;
		y1 = y1 + 26;
		y2 = y2 + 26;
		break;
	}
	HAL_PAINT_CS_L;
    hal_paint_send_command ( 0x2A );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( x1 >> 8);
	hal_paint_send_8bit ( x1  );
	hal_paint_send_8bit ( x2 >> 8 );
	hal_paint_send_8bit ( x2  );
	hal_paint_send_command ( 0x2B );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( y1 >> 8 );
	hal_paint_send_8bit ( y1 );
	hal_paint_send_8bit ( y2 >> 8 );
	hal_paint_send_8bit ( y2 );
}


void CODE hal_paint_set_pixel_color (COORD x, COORD y, COLOR color)
{
    if ((x >= hal_paint_get_width()) || (y >= hal_paint_get_height()))
        return;
    if ((x < 0) || (y < 0))
        return;
    screen_buf [ (y * hal_paint_get_width()) + x] = color;
    /*
    hal_paint_set_window (x, x, y, y);
	hal_paint_send_command ( 0x2C );
	HAL_PAINT_DC_H;
	hal_paint_send_8bit ( color >> 8 );
	hal_paint_send_8bit ( (U8)color );
	HAL_PAINT_CS_H;
	_NOP();//os_delay_us (HAL_PAINT_DELAY);
	*/
}


COLOR CODE hal_paint_get_pixel (COORD x, COORD y)
{
	if ((x >= hal_paint_get_width()) || (y >= hal_paint_get_height()))
		return COLOR_BLACK;
	if ((x < 0) || (y < 0))
		return COLOR_BLACK;
    return screen_buf [ (y * hal_paint_get_width()) + x];
}


void hal_paint_update (void)
{
    U32 i, j;
    U16 buf [ 32 ];
    
#if SCREEN_ENABLE_PARTIAL_UPDATE
    hal_paint_set_window (0, hal_paint_get_width() - 1, 0, hal_paint_get_height() - 1);
   	hal_paint_send_command ( 0x2C );
   	HAL_PAINT_DC_H;
   	i = 0;
    while (i < (SCREEN_VIRTUAL_W * SCREEN_VIRTUAL_H+1))
    {
		for (j = 0; j < 32; j++)
		{
			buf[j] = screen_buf [ i ];
			i = i + 1;
		}
		hal_spi_send_512bit (HSPI, buf); //we send 64 bytes -> 32 colors
	}
	HAL_PAINT_CS_H;
#else
    hal_paint_set_window (0, hal_paint_get_width() - 1, 0, hal_paint_get_height() - 1);
   	hal_paint_send_command ( 0x2C );
   	HAL_PAINT_DC_H;
    hal_spi_send_all (HSPI, &screen_buf[0] );
	HAL_PAINT_CS_H;
#endif
}


void CODE hal_paint_fill_block_color (COORD x, COORD y, COORD w, COORD h, COLOR color)
{
    COORD xx, yy;
    
    for (xx = 0; xx < w; xx++)
    {
        for (yy = 0; yy < h; yy++)
        {
            hal_paint_set_pixel_color (x + xx, y + yy, color);
        }
    }
    /*
	hal_paint_set_window (x, x + (w - 1), y, y + (h - 1));
    hal_paint_send_command ( 0x2C );
	HAL_PAINT_DC_H;
	i = (w * h);
	while (i--)
    {
    	hal_paint_send_8bit ( color >> 8 );
    	hal_paint_send_8bit ( (U8)color );
    }
    HAL_PAINT_CS_H;
	_NOP();//os_delay_us (HAL_PAINT_DELAY);
	*/
}


void CODE hal_paint_fill_block (COORD x, COORD y, COORD w, COORD h, COLOR *buf)
{
    COORD xx, yy;
    if (h > 0)
    {
		for (xx = 0; xx < w; xx++)
		{
			for (yy = 0; yy < h; yy++)
			{
				hal_paint_set_pixel_color (x + xx, y + yy, *buf++);
			}
		}
    }
    if (h < 0)
    {
    	y = y + h * (-1);
    	for (xx = 0; xx < w; xx++)
		{
			for (yy = 0; yy > h; yy--)
			{
				hal_paint_set_pixel_color (x + xx, y + yy, *buf++);
			}
		}
    }
	/*
    hal_paint_set_window (x, x + (w - 1), y, y + (h - 1));
    hal_paint_send_command ( 0x2C );
	HAL_PAINT_DC_H;
	i = (w * h);
    while (i--)
    {
    	COLOR color = *buf++;
    	hal_paint_send_8bit ( color >> 8 );
    	hal_paint_send_8bit ( (U8)color );
    }
    HAL_PAINT_CS_H;
	_NOP();//os_delay_us (HAL_PAINT_DELAY);
	*/

}


void CODE hal_paint_clear_screen (COLOR color)
{
	//hal_paint_fill_block_color ( 0, 0, SCREEN_VIRTUAL_W - 1, SCREEN_VIRTUAL_H - 1, color );
	U32 i;
	COLOR *p = &screen_buf[0];
    for (i = 0; i < (SCREEN_VIRTUAL_W * SCREEN_VIRTUAL_H); i++)
    {
    	*p++ = color;
    }
}


void CODE hal_paint_set_contrast (U8 value)
{
}


void CODE hal_paint_set_orientation (U8 orientation)
{
	HAL_PAINT_CS_L;
	switch (orientation)
	{
	case SCREEN_ORIENTATION_0:
    	hal_paint_send_command ( 0x36 );
    	HAL_PAINT_DC_H;
    	hal_paint_send_8bit ( 0x08 ); //MY=0, MX=0, MV=0
    	hal_screen_s.orientation = SCREEN_ORIENTATION_0;
    	break;

	case SCREEN_ORIENTATION_90:
		hal_paint_send_command ( 0x36 );
		HAL_PAINT_DC_H;
		hal_paint_send_8bit ( 0x68 ); //MY=0, MX=1, MV=1
		hal_screen_s.orientation = SCREEN_ORIENTATION_90;
		break;

	case SCREEN_ORIENTATION_180:
		hal_paint_send_command ( 0x36 );
		HAL_PAINT_DC_H;
		hal_paint_send_8bit ( 0xC8 ); //MY=1, MX=1, MV=0
		hal_screen_s.orientation = SCREEN_ORIENTATION_180;
		break;

	case SCREEN_ORIENTATION_270:
		hal_paint_send_command ( 0x36 );
		HAL_PAINT_DC_H;
		hal_paint_send_8bit ( 0xA8 ); //MY=1, MX=0, MV=1
		hal_screen_s.orientation = SCREEN_ORIENTATION_270;
		break;

	default: break;
	}
	HAL_PAINT_CS_H;
}


COORD CODE hal_paint_get_width(void)
{
	switch (hal_screen_s.orientation)
	{
	case SCREEN_ORIENTATION_0:
	case SCREEN_ORIENTATION_180:
		return SCREEN_VIRTUAL_W;
		break;
	case SCREEN_ORIENTATION_90:
	case SCREEN_ORIENTATION_270:
		return SCREEN_VIRTUAL_H;
		break;
	}
    return 0;
}


COORD CODE hal_paint_get_height(void)
{
	switch (hal_screen_s.orientation)
	{
	case SCREEN_ORIENTATION_0:
	case SCREEN_ORIENTATION_180:
		return SCREEN_VIRTUAL_H;
		break;
	case SCREEN_ORIENTATION_90:
	case SCREEN_ORIENTATION_270:
		return SCREEN_VIRTUAL_W;
		break;
	}
    return 0;
}

#endif
