/*
 * Paulo Pedreiras, 2022/09
 *
 * Demo of the use of the UART driver.
 *      There are several APIs. The demo is based on the asynchronous API, which is very flexible and covers most of the use cases.
 *      The demo configures the UART and echoes the user input.
 *      Received characters are accumulated during a period set by MAIN_SLEEP_TIME_MS
 *      All UART events are echoed
 *
 * Info source:
 *      https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/reference/peripherals/uart.html?highlight=uart_configure#overview
 *
 * prj.conf should be set as follows:
 *
 *      To enable UART
 *          CONFIG_SERIAL=y
 *          CONFIG_UART_ASYNC_API=y
 *
 *
 */
/* Includes. Add according to the resources used  */
#include <zephyr/kernel.h>        /* for k_msleep() */
#include <zephyr/device.h>        /* for device_is_ready() and device structure */
#include <zephyr/devicetree.h>    /* for DT_NODELABEL() */
#include <zephyr/drivers/uart.h>  /* for ADC API*/
#include <zephyr/drivers/gpio.h>  /* for GPIO API*/
#include <zephyr/sys/printk.h>    /* for printk()*/
#include <stdio.h>                /* for sprintf() */
#include <zephyr/timing/timing.h> /* Kernel timing services */
#include <zephyr/drivers/i2c.h>   /* Required for  I2C */
#include <zephyr/sys/sem.h>
#include <stdlib.h>
#include <string.h>
#include "uart/uart.h"

#define UART_NODE DT_NODELABEL(uart0) /* UART0 node ID*/
#define MAIN_SLEEP_TIME_MS 10         /* Time between main() activations */

#define FATAL_ERR -1 /* Fatal error return code, app terminates */

#define RXBUF_SIZE 60   /* RX buffer size */
#define TXBUF_SIZE 60   /* TX buffer size */
#define RX_TIMEOUT 1000 /* Inactivity period after the instant when last char was received that triggers an rx event (in us) */

/* Size of stack area used by each thread (can be thread specific, if necessary)*/
#define STACK_SIZE 8192

/* Thread scheduling priority */
#define UART_prio 1
#define LEDS_prio 1
#define BTNS_prio 1
#define TC74_prio 1

/* Thread periodicity (in ms)*/
#define UART_period 1000
#define LEDS_period 2000
#define BTNS_period 4000
#define TC74_period 4000

/* Macros */
#define ACK_MSG_SIZE 9

/* IO Setup */
// LED nodes
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)
static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(LED0_NODE, gpios),
    GPIO_DT_SPEC_GET(LED1_NODE, gpios),
    GPIO_DT_SPEC_GET(LED2_NODE, gpios),
    GPIO_DT_SPEC_GET(LED3_NODE, gpios)};

// Button nodes
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)
static const struct gpio_dt_spec buttons[] = {
    GPIO_DT_SPEC_GET(SW0_NODE, gpios),
    GPIO_DT_SPEC_GET(SW1_NODE, gpios),
    GPIO_DT_SPEC_GET(SW2_NODE, gpios),
    GPIO_DT_SPEC_GET(SW3_NODE, gpios)};

// Button call back
static struct gpio_callback button_cb_data;

static unsigned char buttons_pressed = 0x0;

/* Create thread stack space */
K_THREAD_STACK_DEFINE(UART_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(LEDS_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(BTNS_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(TC74_stack, STACK_SIZE);

/* Create variables for thread data */
struct k_thread UART_data;
struct k_thread LEDS_data;
struct k_thread BTNS_data;
struct k_thread TC74_data;

/* Create task IDs */
k_tid_t UART_tid;
k_tid_t LEDS_tid;
k_tid_t BTNS_tid;
k_tid_t TC74_tid;

/* Thread code prototypes */
void button_pressed(struct gpio_callback *);
void UART_code(RTDB *db, void *argB, void *argC);
void LEDS_code(RTDB *db, void *argB, void *argC);
void BTNS_code(RTDB *db, void *argB, void *argC);
void TC74_code(RTDB *db, void *argB, void *argC);

/* Define timers for tasks activations */
K_TIMER_DEFINE(UART_timer, NULL, NULL);
K_TIMER_DEFINE(LEDS_timer, NULL, NULL);
K_TIMER_DEFINE(BTNS_timer, NULL, NULL);
K_TIMER_DEFINE(TC74_timer, NULL, NULL);

/* Struct for UART configuration (if using default values is not needed) */
const struct uart_config uart_cfg = {
    .baudrate = 115200,
    .parity = UART_CFG_PARITY_NONE,
    .stop_bits = UART_CFG_STOP_BITS_1,
    .data_bits = UART_CFG_DATA_BITS_8,
    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

/* UART related variables */
const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
static uint8_t rx_buf[RXBUF_SIZE];   /* RX buffer, to store received data */
static uint8_t rx_chars[RXBUF_SIZE]; /* chars actually received  */
volatile int uart_rxbuf_nchar = 0;   /* Number of chars currrntly on the rx buffer */
char errCode;

#define RETRANSMISSION_DELAY_MS 5000 /* Time between retransmissions*/

/* UART callback function prototype */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);

/* TC74 sensor-related defines */
#define TC74_ADDR 0x4D /* TC74 Address *** NOTE: CHECK THE SENSOR DATASHEET ADDRESS CAN BE DIFFERENT*** */
                       /* Do not forget to make sure that this address in set on the overlay file      */
                       /*    as the sensor address is obtained from the DT */

#define TC74_CMD_RTR 0x00  /* Read temperature command */
#define TC74_CMD_RWCR 0x01 /* Read/write configuration register */

/* I2C device vars and defines */
#define I2C0_NID DT_NODELABEL(tc74sensor)
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C0_NID);

/* Other defines*/
#define TC74_UPDATE_PERIOD_MS 1000 /* Time between temperature readings */

#define DEBUG 0 /* set to one to toggle printks */

/* Main function */
void main(void)
{

    RTDB *db = rtdb_create();
    UART_tid = k_thread_create(&UART_data, UART_stack,
                               K_THREAD_STACK_SIZEOF(UART_stack), UART_code,
                               db, NULL, NULL, UART_prio, 0, K_NO_WAIT);

    TC74_tid = k_thread_create(&TC74_data, TC74_stack,
                               K_THREAD_STACK_SIZEOF(TC74_stack), TC74_code,
                               db, NULL, NULL, TC74_prio, 0, K_NO_WAIT);

    LEDS_tid = k_thread_create(&LEDS_data, LEDS_stack,
                               K_THREAD_STACK_SIZEOF(LEDS_stack), LEDS_code,
                               db, NULL, NULL, LEDS_prio, 0, K_NO_WAIT);

    BTNS_tid = k_thread_create(&BTNS_data, BTNS_stack,
                               K_THREAD_STACK_SIZEOF(BTNS_stack), BTNS_code,
                               db, NULL, NULL, BTNS_prio, 0, K_NO_WAIT);
}

/* UART callback implementation */
/* Note that callback functions are executed in the scope of interrupt handlers. */
/* They run asynchronously after hardware/software interrupts and have a higher priority than all threads */
/* Should be kept as short and simple as possible. Heavier processing should be deferred to a task with suitable priority*/
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    int err;

    switch (evt->type)
    {

    case UART_TX_DONE:
        if (DEBUG)
            printk("UART_TX_DONE event \n\r");
        break;

    case UART_TX_ABORTED:
        if (DEBUG)
            printk("UART_TX_ABORTED event \n\r");
        break;

    case UART_RX_RDY:
        if (DEBUG)
            printk("UART_RX_RDY event \n\r");
        /* Just copy data to a buffer. Usually it is preferable to use e.g. a FIFO to communicate with a task that shall process the messages*/
        memcpy(&rx_chars[uart_rxbuf_nchar], &(rx_buf[evt->data.rx.offset]), evt->data.rx.len);
        uart_rxbuf_nchar++;
        break;

    case UART_RX_BUF_REQUEST:
        if (DEBUG)
            printk("UART_RX_BUF_REQUEST event \n\r");
        break;

    case UART_RX_BUF_RELEASED:
        if (DEBUG)
            printk("UART_RX_BUF_RELEASED event \n\r");
        break;

    case UART_RX_DISABLED:
        /* When the RX_BUFF becomes full RX is is disabled automaticaly.  */
        /* It must be re-enabled manually for continuous reception */
        if (DEBUG)
            printk("UART_RX_DISABLED event \n\r");
        err = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RX_TIMEOUT);
        if (err)
        {
            printk("uart_rx_enable() error. Error code:%d\n\r", err);
            exit(FATAL_ERR);
        }
        break;

    case UART_RX_STOPPED:
        if (DEBUG)
            printk("UART_RX_STOPPED event \n\r");
        break;

    default:
        if (DEBUG)
            printk("UART: unknown event \n\r");
        break;
    }
}

char *rcv_msg(char *buffer, int i)
{
    int k = 0;
    char *msg = malloc(sizeof(char) * 30);

    // Read one command
    while (buffer[i] != '!')
    {
        msg[k] = buffer[i];

        if (-1 == --i)
            i = 63;

        if (29 == ++k)
        {
            if (DEBUG)
                printk("message too long: %d\n", k);
            memset(msg, 0, 30);
            errCode = '4';
            break;
        }

        if (buffer[i] == '#')
        {
            if (DEBUG)
                printk("bad message\n");
            memset(msg, 0, 30);
            errCode = '4';
            break;
        }
    }

    // Add '!' to the end of the command message
    msg[k] = '!';
    msg[++k] = '\0';

    // Reverse the command message
    int j = 0, l = strlen(msg) - 1;
    char temp;
    while (j < l)
    {
        temp = msg[j];
        msg[j] = msg[l];
        msg[l] = temp;
        l--, j++;
    }

    return msg;
}

void UART_code(RTDB *db, void *argB, void *argC)
{

    /* Local vars */
    int err = 0; /* Generic error variable */
    uint8_t rep_mesg[TXBUF_SIZE];

    /* Configure UART */
    err = uart_configure(uart_dev, &uart_cfg);
    if (err == -ENOSYS)
    { /* If invalid configuration */
        printk("uart_configure() error. Invalid configuration\n\r");
        return;
    }

    /* Register callback */
    err = uart_callback_set(uart_dev, uart_cb, NULL);
    if (err)
    {
        printk("uart_callback_set() error. Error code:%d\n\r", err);
        return;
    }

    /* Enable data reception */
    err = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RX_TIMEOUT);
    if (err)
    {
        printk("uart_rx_enable() error. Error code:%d\n\r", err);
        return;
    }

    char buffer[64] = {0};
    int i = 0;
    char retransmission[70];
    int64_t fin_time = 0, release_time = 0;

    // Flag to wait for ACK
    int waitAck = 0;

    /* Main loop */
    while (1)
    {
        k_msleep(MAIN_SLEEP_TIME_MS);
        if (waitAck)
        {
            fin_time = k_uptime_get();
            if (fin_time > release_time)
            {
                release_time = fin_time + RETRANSMISSION_DELAY_MS;
                char clearLine[] = "\33[2K\r"; /* if the user is already writing the ack message erase it, send retransmission and send incomplete message*/
                err = uart_tx(uart_dev, clearLine, strlen(clearLine), SYS_FOREVER_MS);
                if (err)
                {
                    printk("uart_tx() error. Error code:%d\n\r", err);
                    return;
                }
                k_msleep(10);
                err = uart_tx(uart_dev, retransmission, strlen(retransmission), SYS_FOREVER_MS);
                if (err)
                {
                    printk("uart_tx() error. Error code:%d\n\r", err);
                    return;
                }
                k_msleep(10);
                if (strlen(buffer) > 0)
                {
                    err = uart_tx(uart_dev, buffer, strlen(buffer), SYS_FOREVER_MS);
                    if (err)
                    {
                        printk("uart_tx() error. Error code:%d\n\r", err);
                        return;
                    }
                    k_msleep(10);
                }
            }
        }

        if (uart_rxbuf_nchar > 0)
        {
            rx_chars[uart_rxbuf_nchar] = 0; /* Terminate the string */
            uart_rxbuf_nchar = 0;           /* Reset counter */

            for (int k = 0; k < strlen(rx_chars); k++)
                buffer[i] = rx_chars[k], i = ++i % 64;

            sprintf(rep_mesg, "%s", rx_chars);
            err = uart_tx(uart_dev, rep_mesg, strlen(rep_mesg), SYS_FOREVER_MS);
            if (err)
            {
                printk("uart_tx() error. Error code:%d\n\r", err);
                return;
            }
            k_msleep(1);

            if (strchr(buffer, '#') != NULL)
            {
                char newLine[] = "\r\n";
                err = uart_tx(uart_dev, newLine, strlen(newLine), SYS_FOREVER_MS);
                if (err)
                {
                    printk("uart_tx() error. Error code:%d\n\r", err);
                    return;
                }
                k_msleep(1);

                i--;
                errCode = '1';
                char *msg = rcv_msg(buffer, i);
                memset(buffer, 0, 64);
                i = 0;

                if (waitAck)
                {
                    if (uart_checkSum(msg) && msg[2] == 'Z' && msg[3] == retransmission[2] && msg[4] == '1')
                        waitAck = 0;
                    else
                    {
                        release_time = k_uptime_get() + RETRANSMISSION_DELAY_MS;
                        err = uart_tx(uart_dev, retransmission, strlen(retransmission), SYS_FOREVER_MS);
                        if (err)
                        {
                            printk("uart_tx() error. Error code:%d\n\r", err);
                            return;
                        }
                        k_msleep(1);
                    }
                }

                else
                {
                    // Send ACK
                    char ack[30] = "!1Z";
                    char id[2] = {msg[2], '\0'};
                    strcat(ack, id);
                    if (errCode == '4')
                    {
                        strcat(ack, "4");
                        uart_apply_checksum(ack, ACK_MSG_SIZE);
                        strcat(ack, "\r\n");
                        err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                        if (err)
                        {
                            printk("uart_tx() error. Error code:%d\n\r", err);
                            return;
                        }
                        k_msleep(1);
                    }

                    else if (msg[2] < '0' || msg[2] > '7')
                    {
                        strcat(ack, "2");
                        uart_apply_checksum(ack, ACK_MSG_SIZE);
                        strcat(ack, "\r\n");
                        err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                        if (err)
                        {
                            printk("uart_tx() error. Error code:%d\n\r", err);
                            return;
                        }
                        k_msleep(1);
                    }

                    else
                    {
                        if (uart_checkSum(msg)) // Checksum is correct
                        {
                            strcat(ack, "1");
                            uart_apply_checksum(ack, ACK_MSG_SIZE);
                            strcat(ack, "\r\n");
                            err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                            if (err)
                            {
                                printk("uart_tx() error. Error code:%d\n\r", err);
                                return;
                            }
                            k_msleep(1);
                            char *response = uart_interface(db, msg);

                            if (strlen(response) > 5)
                            {
                                err = uart_tx(uart_dev, response, strlen(response), SYS_FOREVER_MS);
                                if (err)
                                {
                                    printk("uart_tx() error. Error code:%d\n\r", err);
                                    return;
                                }
                                k_msleep(1);

                                waitAck = 1;
                                release_time = k_uptime_get() + RETRANSMISSION_DELAY_MS;
                                memcpy(retransmission, response, strlen(response) + 1);
                                free(response);
                            }
                        }

                        else
                        {
                            strcat(ack, "3");
                            uart_apply_checksum(ack, ACK_MSG_SIZE);
                            strcat(ack, "\r\n");
                            err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                            if (err)
                            {
                                printk("uart_tx() error. Error code:%d\n\r", err);
                                return;
                            }
                            k_msleep(1);
                        }
                        memset(msg, 0, 30);
                    }
                }
                free(msg);
            }

            // it should only go up to 63 due to buffer size
            // i = ++i % 64;
        }
        // printk(".\n");
    }
}

void LEDS_code(RTDB *db, void *argB, void *argC)
{
    int ret[4];

    // Check if GPIO devices are ready
    for (int i = 0; i < 4; i++)
        if (!device_is_ready(leds[i].port))
        {
            printk("Error: GPIO %d output devices not ready\n", i);
            return;
        }

    // Check if GPIO devices are configured correctly
    for (int i = 0; i < 4; i++)
        if (gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE) < 0)
        {
            printk("Error: GPIO %d output devices not configured\n", i);
            return;
        }

    while (1)
    {
        k_msleep(LEDS_period);
        unsigned char o = rtdb_get_outputs(db);
        if (DEBUG)
            printk("leds: %x\n", o);

        // Set LEDs to match rtdb values
        for (int i = 0; i < 4; i++)
            gpio_pin_set(leds[i].port, leds[i].pin, o & (0x01 << i));
    }
}

void BTNS_code(RTDB *db, void *argB, void *argC)
{
    // Check if GPIO devices are ready
    for (int i = 0; i < 4; i++)
        if (!device_is_ready(buttons[i].port))
        {
            printk("Error: GPIO %d output devices not ready\n", i);
            return;
        }

    // Check if GPIO devices are configured correctly
    for (int i = 0; i < 4; i++)
        if (gpio_pin_configure_dt(&buttons[i], GPIO_INPUT) < 0)
        {
            printk("Error: GPIO %d output devices not configured\n", i);
            return;
        }

    // Configure GPIO devices' interrupts
    for (int i = 0; i < 4; i++)
        if (gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE) < 0)
        {
            printk("Error: GPIO %d interrupts not properly configured\n", i);
            return;
        }

    // Setup callback functions
    gpio_init_callback(&button_cb_data, button_pressed, BIT(buttons[0].pin) | BIT(buttons[1].pin) | BIT(buttons[2].pin) | BIT(buttons[3].pin));
    for (int i = 0; i < 4; i++)
        gpio_add_callback(buttons[i].port, &button_cb_data);

    while (1)
    {
        k_msleep(BTNS_period);
        rtdb_set_inputs(db, buttons_pressed);
        buttons_pressed = 0x0;
    }
}

void TC74_code(RTDB *db, void *argB, void *argC)
{
    int ret = 0;      /* General return variable */
    uint8_t temp = 0; /* Temperature value (raw read from sensor)*/

    if (!device_is_ready(dev_i2c.bus))
    {
        printk("I2C bus %s is not ready!\n\r", dev_i2c.bus->name);
        return;
    }
    else
    {
        printk("I2C bus %s, device address %x ready\n\r", dev_i2c.bus->name, dev_i2c.addr);
    }

    /* Write (command RTR) to set the read address to temperature */
    /* Only necessary if a config done before (not the case), but let's stay in the safe side */
    ret = i2c_write_dt(&dev_i2c, TC74_CMD_RTR, 1);
    if (ret != 0)
    {
        printk("Failed to write to I2C device at address %x, register %x\n\r", dev_i2c.addr, TC74_CMD_RTR);
    }
    while (1)
    {
        /* Read temperature register */
        ret = i2c_read_dt(&dev_i2c, &temp, sizeof(temp));
        if (ret != 0)
        {
            printk("Failed to read from I2C device at address %x, register  at Reg. %x\n\r", dev_i2c.addr, TC74_CMD_RTR);
        }

        rtdb_insert_temp(db, temp);

        /* Pause  */
        k_msleep(TC74_UPDATE_PERIOD_MS);
    }
}

void button_pressed(struct gpio_callback *cb)
{
    for (int i = 0; i < 4; i++)
        if (gpio_pin_get(buttons[i].port, buttons[i].pin))
        {
            buttons_pressed |= 0x01 << i;
            if (DEBUG)
                printk("Button %d pressed\n", i + 1);
        }
}