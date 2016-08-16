#ifdef CFG_PM

#include "hardware_conf.h"
#include "rtos.h"
#include "types.h"
#include "pm.h"

static int _pm_lock = 0;

static xSemaphoreHandle pm_mutex;

#ifdef CFG_DEEP_SLEEP
//static int toggle;
//XTAL = 18,432MHz 
//MCK = (18,432/10)*52 = 95,8464MHz  MCK: 47,9232

//------------------------------------------------------------------------------
//         Internal definitions
//------------------------------------------------------------------------------
// Startup time of main oscillator (in number of slow clock ticks).
#define BOARD_OSCOUNT           (AT91C_CKGR_OSCOUNT & (0x40 << 8))

// USB PLL divisor value to obtain a 48MHz clock. DIV 1=/2
#define BOARD_USBDIV            AT91C_CKGR_USBDIV_1  

// PLL frequency range.
#define BOARD_CKGR_PLL          AT91C_CKGR_OUT_0

// PLL startup time (in number of slow clock ticks).
#define BOARD_PLLCOUNT          (16 << 8)

// PLL MUL value.
#define BOARD_MUL               (AT91C_CKGR_MUL & (51 << 16))

// PLL DIV value.
#define BOARD_DIV               (AT91C_CKGR_DIV & 10)

// Master clock prescaler value.
#define BOARD_PRESCALER         AT91C_PMC_PRES_CLK_2

//------------------------------------------------------------------------------
/// Put the CPU in 32kHz, disable PLL, main oscillator
/// Put voltage regulator in standby mode
//------------------------------------------------------------------------------
static void LowPowerMode(void)
{
    //Led_setState(&led, 1);
    //TRACE_INFO("LowPowerMode %d %d %x\r\n\n\n", AT91C_BASE_RTTC->RTTC_RTAR, AT91C_BASE_RTTC->RTTC_RTVR, AT91C_BASE_RTTC->RTTC_RTMR);

    //taskENTER_CRITICAL();
    // MCK=48MHz to MCK=32kHz
    // MCK = SLCK/2 : change source first from 48 000 000 to 18. / 2 = 9M
    AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_PRES_CLK_2;
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY ) );
    // MCK=SLCK : then change prescaler
    AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_CSS_SLOW_CLK;
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY ) );
    // disable PLL
    AT91C_BASE_PMC->PMC_PLLR = 0;
    // Disable Main Oscillator
    AT91C_BASE_PMC->PMC_MOR = 0;

    // Voltage regulator in standby mode : Enable VREG Low Power Mode
    AT91C_BASE_VREG->VREG_MR |= AT91C_VREG_PSTDBY;
    //taskEXIT_CRITICAL();

    PMC_DisableProcessorClock();
}

//------------------------------------------------------------------------------
/// Put voltage regulator in normal mode
/// Return the CPU to normal speed 48MHz, enable PLL, main oscillator
//------------------------------------------------------------------------------
void NormalPowerMode(void)
{
    // Voltage regulator in normal mode : Disable VREG Low Power Mode
    AT91C_BASE_VREG->VREG_MR &= ~AT91C_VREG_PSTDBY;

    // MCK=32kHz to MCK=48MHz
    // enable Main Oscillator
#if 1
    AT91C_BASE_PMC->PMC_MOR = (( (AT91C_CKGR_OSCOUNT & (0x06 <<8)) | AT91C_CKGR_MOSCEN ));
#else
    AT91C_BASE_PMC->PMC_MOR = BOARD_OSCOUNT | AT91C_CKGR_MOSCEN;
#endif
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCS ) );

    // enable PLL@96MHz
    /* Initialize PLL at 96MHz (96.109) and USB clock to 48MHz */
#if 1
    AT91C_BASE_PMC->PMC_PLLR = ((AT91C_CKGR_DIV & 0x0E) |
         (AT91C_CKGR_PLLCOUNT & (28<<8)) |
         (AT91C_CKGR_MUL & (0x48<<16)));
#else
    AT91C_BASE_PMC->PMC_PLLR = BOARD_USBDIV | BOARD_CKGR_PLL | BOARD_PLLCOUNT
                               | BOARD_MUL | BOARD_DIV;
#endif
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCK ) );

    /* Wait for the master clock if it was already initialized */
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY ) );

    AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1 ;

    // MCK=SLCK/2 : change prescaler first
    AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_PRES_CLK_2;
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY ) );

    // MCK=PLLCK/2 : then change source
    AT91C_BASE_PMC->PMC_MCKR |= AT91C_PMC_CSS_PLL_CLK  ;
    while( !( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY ) );
    //Led_setState(&led, 0);
}

int check_power_mode(void) {

    //taskENTER_CRITICAL();
    //taskDISABLE_INTERRUPTS();
    //if (!(AT91C_BASE_PMC->PMC_MCKR & AT91C_PMC_CSS_PLL_CLK)) {
    if ((AT91C_BASE_PMC->PMC_MCKR & AT91C_PMC_CSS_PLL_CLK) && ( AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY )) {
        return 0;
    }
    uint32_t ticks = Timer_tick_count();
    uint32_t mkcr = AT91C_BASE_PMC->PMC_MCKR;
    uint32_t sr = AT91C_BASE_PMC->PMC_SR;
    NormalPowerMode();
    //TRACE_INFO("NormalPowerMode %d %x %x\r\n", Timer_tick_count() - ticks, mkcr, sr);
    //taskEXIT_CRITICAL();
    //taskENABLE_INTERRUPTS();

    return 1;
}
#endif

int pm_init(void) {
    pm_mutex = xSemaphoreCreateMutex();

    if (pm_mutex == NULL) {
        panic("pm_init");
        return -1;
    }

    return 0;
}

int pm_close(void) {
    vQueueDelete(pm_mutex);
    return 0;
}

void pm_lock() {
    xSemaphoreTake(pm_mutex, -1);
    _pm_lock++;
    xSemaphoreGive(pm_mutex);
}

void pm_unlock() {
    xSemaphoreTake(pm_mutex, -1);
    _pm_lock--;
    if (_pm_lock < 0) {
        panic("pm_unlock");
    }
    xSemaphoreGive(pm_mutex);
}

void pm_unlock_isr() {
    _pm_lock--;
    if (_pm_lock < 0) {
        panic("pm_unlock");
    }
}

uint32_t total_time_in_sleep = 0;
uint32_t total_time_in_deep_sleep = 0;

void vApplicationIdleHook( void )
{
    //TRACE_INFO("vApplicationIdleHook\r\n");
    //toggle = !toggle; // prevent the function from being optimized away?

    taskENTER_CRITICAL();
    vTaskSuspendAll();
    // disable PIT (FreeRTOS ticks)
    //AT91C_BASE_PITC->PITC_PIMR &= ~AT91C_PITC_PITEN;
#ifdef CFG_SCHEDULER_RTT
    AT91C_BASE_RTTC->RTTC_RTMR &= ~AT91C_RTTC_RTTINCIEN;
#else
    AT91C_BASE_PITC->PITC_PIMR &= ~AT91C_PITC_PITIEN;
#endif

    /* disable CPU clock
        Task_sleep() can't be used
        Semaphore/Queue get funcs with timeout can't be used
    */
    //uint32_t sleep_time = Timer_tick_count_nonblock();
    //uint32_t sleep_time = Timer_tick_count_nonblock2();
    uint32_t sleep_time = Timer_tick_count_nonblock3();
    //OK uint32_t sleep_time = Timer_tick_count();

    //TRACE_INFO("going to sleep %d\r\n", sleep_time);
    //OK Timer_tick_count_sleep();
    //AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_PCK;

    bool deep_sleep;
#ifdef CFG_DEEP_SLEEP
    int32_t t = AT91C_BASE_RTTC->RTTC_RTAR - AT91C_BASE_RTTC->RTTC_RTVR;
    if (_pm_lock || (t > 0 && t < 100)) {
        deep_sleep = false;
        //taskENTER_CRITICAL();
        PMC_DisableProcessorClock();
        taskEXIT_CRITICAL();
    } else {
        deep_sleep = true;
        //taskENTER_CRITICAL();
        LowPowerMode();
        taskEXIT_CRITICAL();
    }
#else
    deep_sleep = false;
    PMC_DisableProcessorClock();
    taskEXIT_CRITICAL();
#endif
    // CPU in idle mode now, waiting for IRQ

    //OK Timer_tick_count_wakeup();

    //uint32_t wakeup_time = Timer_tick_count_nonblock();
    uint32_t wakeup_time = Timer_tick_count_nonblock3();
    //uint32_t wakeup_time = Timer_tick_count_nonblock2();
    //OK uint32_t wakeup_time = Timer_tick_count();

    uint32_t time_in_sleep = wakeup_time - sleep_time;
    if (deep_sleep) {
        total_time_in_deep_sleep += time_in_sleep;
    } else {
        total_time_in_sleep += time_in_sleep;
    }
    Timer_tick_count_wakeup(time_in_sleep);
    //TRACE_INFO("waking up %d %d %d %d%%\r\n", wakeup_time, time_in_sleep, total_time_in_sleep, total_time_in_sleep * 100 / wakeup_time);
    // enable PIT (FreeRTOS ticks)
#ifdef CFG_SCHEDULER_RTT
    AT91C_BASE_RTTC->RTTC_RTMR |= AT91C_RTTC_RTTINCIEN;
#else
    AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITIEN;
#endif
    xTaskResumeAll();
}
#else
void pm_lock() {
}

void pm_unlock() {
}

int check_power_mode(void) {
    return 0;
}

int pm_init(void) {
    return 0;
}

int pm_close(void) {
    return 0;
}

#endif
