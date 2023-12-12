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
// Buttons
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec but0 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

#define SW1_NODE DT_ALIAS(sw1)
static const struct gpio_dt_spec but1 = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});

#define SW2_NODE DT_ALIAS(sw2)
static const struct gpio_dt_spec bu22 = GPIO_DT_SPEC_GET_OR(SW2_NODE, gpios, {0});

#define SW3_NODE DT_ALIAS(sw3)
static const struct gpio_dt_spec but3 = GPIO_DT_SPEC_GET_OR(SW3_NODE, gpios, {0});

// LEDs
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0});

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0});

#define LED2_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET_OR(LED2_NODE, gpios, {0});

#define LED3_NODE DT_ALIAS(led3)
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET_OR(LED3_NODE, gpios, {0});

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
void UART_code(void *argA, void *argB, void *argC);
void LEDS_code(RTDB *db, void *argB, void *argC);
void BTNS_code(void *argA, void *argB, void *argC);
void TC74_code(void *argA, void *argB, void *argC);

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
struct k_sem uart_sem;

/* UART callback function prototype */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);

/* Main function */
void main(void)
{
    UART_tid = k_thread_create(&UART_data, UART_stack,
                               K_THREAD_STACK_SIZEOF(UART_stack), UART_code,
                               NULL, NULL, NULL, UART_prio, 0, K_NO_WAIT);

    while (1)
    {
        k_msleep(1000);
        // char response[] = "hello alive hello alive hello alive hello alive hello alive hello alive\n";
        // int err = uart_tx(uart_dev, response, strlen(response), SYS_FOREVER_MS);
        // if (err) {
        //     printk("uart_tx() error. Error code:%d\n\r",err);
        //     k_sem_give(&uart_sem);
        //     return;
        // }
    }
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
        printk("UART_TX_DONE event \n\r");
        break;

    case UART_TX_ABORTED:
        printk("UART_TX_ABORTED event \n\r");
        break;

    case UART_RX_RDY:
        printk("UART_RX_RDY event \n\r");
        /* Just copy data to a buffer. Usually it is preferable to use e.g. a FIFO to communicate with a task that shall process the messages*/
        memcpy(&rx_chars[uart_rxbuf_nchar], &(rx_buf[evt->data.rx.offset]), evt->data.rx.len);
        uart_rxbuf_nchar++;
        break;

    case UART_RX_BUF_REQUEST:
        printk("UART_RX_BUF_REQUEST event \n\r");
        break;

    case UART_RX_BUF_RELEASED:
        printk("UART_RX_BUF_RELEASED event \n\r");
        break;

    case UART_RX_DISABLED:
        /* When the RX_BUFF becomes full RX is is disabled automaticaly.  */
        /* It must be re-enabled manually for continuous reception */
        printk("UART_RX_DISABLED event \n\r");
        err = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RX_TIMEOUT);
        if (err)
        {
            printk("uart_rx_enable() error. Error code:%d\n\r", err);
            exit(FATAL_ERR);
        }
        break;

    case UART_RX_STOPPED:
        printk("UART_RX_STOPPED event \n\r");
        break;

    default:
        printk("UART: unknown event \n\r");
        break;
    }
}

char *rcv_msg(char *buffer, int i, char *errCode)
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
            printk("message too long: %d\n", k);
            memset(msg, 0, 30);
            memcpy(errCode, "4", 1);
            break;
        }

        if (buffer[i] == '#')
        {
            printk("bad message\n");
            memset(msg, 0, 30);
            memcpy(errCode, "4", 1);
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

void UART_code(void *argA, void *argB, void *argC)
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

    RTDB *db = rtdb_create();

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
                release_time = fin_time + 5000;
                err = uart_tx(uart_dev, retransmission, strlen(retransmission), SYS_FOREVER_MS);
                if (err)
                {
                    printk("uart_tx() error. Error code:%d\n\r", err);
                    return;
                }
                k_msleep(1);
            }
        }

        /* Print string received so far. */
        /* Very basic implementation, just for showing the use of the API */
        /* E.g. it does not prevent race conditions with the callback!!!!*/
        if (uart_rxbuf_nchar > 0)
        {
            rx_chars[uart_rxbuf_nchar] = 0; /* Terminate the string */
            uart_rxbuf_nchar = 0;           /* Reset counter */

            buffer[i] = rx_chars[0];

            sprintf(rep_mesg, "%s", rx_chars);

            if (rx_chars[0] == '#')
            {
                char errCode;
                char *msg = rcv_msg(buffer, i, &errCode);

                if (waitAck)
                {
                    if (uart_checkSum(msg))
                        waitAck = 0;
                    else
                    {
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
                        strcat(ack, "\n\r");
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
                        strcat(ack, "\n\r");
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
                            strcat(ack, "\n\r");
                            err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                            if (err)
                            {
                                printk("uart_tx() error. Error code:%d\n\r", err);
                                return;
                            }
                            char *response = uart_interface(db, msg);

                            err = uart_tx(uart_dev, response, strlen(response), SYS_FOREVER_MS);
                            if (err)
                            {
                                printk("uart_tx() error. Error code:%d\n\r", err);
                                return;
                            }
                            printk("here\n");
                            k_msleep(1);
                            waitAck = 1;
                            release_time = k_uptime_get() + 5000;
                            printk("here\n");
                            memcpy(retransmission, response, strlen(response) + 4);
                            printk("here\n");
                            free(response);
                        }

                        else
                        {
                            strcat(ack, "3");
                            uart_apply_checksum(ack, ACK_MSG_SIZE);
                            strcat(ack, "\n\r");
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
            i = ++i % 64;

            err = uart_tx(uart_dev, rep_mesg, strlen(rep_mesg), SYS_FOREVER_MS);
            if (err)
            {
                printk("uart_tx() error. Error code:%d\n\r", err);
                return;
            }
        }
        // printk(".\n");
    }
}

void LEDS_code(RTDB *db, void *argB, void *argC)
{
    int ret = 0;

    if (!device_is_ready(led0.port) || !device_is_ready(led1.port) || !device_is_ready(led2.port) || !device_is_ready(led3.port))
        return; // If any of the devices is not ready, return

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
        return;

    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
        return;

    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
        return;

    ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
        return;

    gpio_pin_set_dt(&led0, 0);
    gpio_pin_set_dt(&led1, 0);
    gpio_pin_set_dt(&led2, 0);
    gpio_pin_set_dt(&led3, 0);

    while (1)
    {
        k_msleep(LEDS_period);
        char o = rtdb_get_outputs(db);
        gpio_pin_set_dt(&led0, o & 0x01);
        gpio_pin_set_dt(&led1, o & 0x02);
        gpio_pin_set_dt(&led2, o & 0x04);
        gpio_pin_set_dt(&led3, o & 0x08);
    }
}

void BTNS_code(void *argA, void *argB, void *argC) {}
void TC74_code(void *argA, void *argB, void *argC) {}