#include "threadLvgl.h"
#include "Drivers/STM32469I-Discovery/stm32469i_discovery_lcd.h"
#include "Drivers/STM32469I-Discovery/stm32469i_discovery_ts.h"
#include "src/draw/stm32_dma2d/lv_gpu_stm32_dma2d.h"
#include "lvgl_fs_driver.h"

bool ThreadLvgl::refreshEnabled = true;
int ThreadLvgl::refreshTime = 30;

ThreadLvgl::ThreadLvgl(int refreshTimeInit) : m_mainThread(osPriorityBelowNormal)
{
    displayInit();
    touchpadInit();
    lv_fs_stdio_init();
    refreshTime = refreshTimeInit;
    lvTicker.attach(lvTimeCounter, chrono::milliseconds(refreshTime));
    m_mainThread.start(callback(ThreadLvgl::run, this));
}

ThreadLvgl::~ThreadLvgl()
{
    m_mainThread.join();
}

void ThreadLvgl::lvTimeCounter()
{
    lv_tick_inc(refreshTime);
}

void ThreadLvgl::runLvgl()
{
    while (1)
    {
        mutex.lock();
        lv_task_handler();
        mutex.unlock();
        // Call lv_task_handler() periodically every few milliseconds.
        // It will redraw the screen if required, handle input devices etc.
        ThisThread::sleep_for(chrono::milliseconds(refreshTime));
    }
}

void ThreadLvgl::displayInit(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    /*You code here*/
    // Init the touch screen display via the BSP driver. Based on ST's example.
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_DisplayOn();
    BSP_LCD_SelectLayer(0);

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/
    lv_init();
    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     */
    lv_color_t *buf0 = (lv_color_t *)(LCD_FB_START_ADDRESS + 0x00200000); /*A screen sized buffer*/
    lv_color_t *buf1 = (lv_color_t *)(LCD_FB_START_ADDRESS + 0x00400000); /*Another screen sized buffer*/
    static lv_disp_draw_buf_t draw_buf_dsc;
    lv_disp_draw_buf_init(&draw_buf_dsc, buf0, buf1,
                          BSP_LCD_GetXSize() * BSP_LCD_GetYSize()); /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);   /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = BSP_LCD_GetXSize();
    disp_drv.ver_res = BSP_LCD_GetYSize();

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = refreshDisplay;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
void ThreadLvgl::refreshDisplay(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    if (refreshEnabled)
    {
        lv_color_t *fbp32 = (lv_color_t *)LCD_FB_START_ADDRESS;
        lv_draw_stm32_dma2d_buffer_copy(NULL, fbp32 + area->y1 * LV_HOR_RES + area->x1,
                                        (lv_coord_t)LV_HOR_RES, area, color_p, area->x2 - area->x1 + 1, area);
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

/*------------------
 * Touchpad
 * -----------------*/

/*Will be called by the library to read the touchpad*/
void ThreadLvgl::touchpadRead(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    /*Save the pressed coordinates and the state*/
    TS_StateTypeDef TS_State;

    BSP_TS_GetState(&TS_State);
    if (TS_State.touchDetected)
    {
        last_x = TS_State.touchX[0];
        last_y = TS_State.touchY[0];
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

void ThreadLvgl::touchpadInit(void)
{
    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    BSP_TS_Init(420, 272);

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpadRead;
    indevTouchpad = lv_indev_drv_register(&indev_drv);
}
