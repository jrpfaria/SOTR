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
#include <zephyr/kernel.h>          /* for k_msleep() */
#include <zephyr/device.h>          /* for device_is_ready() and device structure */
#include <zephyr/devicetree.h>	    /* for DT_NODELABEL() */
#include <zephyr/drivers/uart.h>    /* for ADC API*/
#include <zephyr/sys/printk.h>      /* for printk()*/
#include <stdio.h>                  /* for sprintf() */
#include <stdlib.h>
#include <string.h>
#include "uart/uart.h"

#define UART_NODE DT_NODELABEL(uart0)   /* UART0 node ID*/
#define MAIN_SLEEP_TIME_MS 10 /* Time between main() activations */ 

#define FATAL_ERR -1 /* Fatal error return code, app terminates */

#define RXBUF_SIZE 60                   /* RX buffer size */
#define TXBUF_SIZE 60                   /* TX buffer size */
#define RX_TIMEOUT 1000                 /* Inactivity period after the instant when last char was received that triggers an rx event (in us) */

/* Struct for UART configuration (if using default values is not needed) */
const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

/* UART related variables */
const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
static uint8_t rx_buf[RXBUF_SIZE];      /* RX buffer, to store received data */
static uint8_t rx_chars[RXBUF_SIZE];    /* chars actually received  */
volatile int uart_rxbuf_nchar=0;        /* Number of chars currrntly on the rx buffer */

/* UART callback function prototype */
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);


/* Main function */
void main(void)
{
    
    /* Local vars */    
    int err=0; /* Generic error variable */
    uint8_t welcome_mesg[] = "UART demo: Type a few chars in a row and then pause for a little while ...\n\r"; 
    uint8_t rep_mesg[TXBUF_SIZE];

    /* Configure UART */
    err = uart_configure(uart_dev, &uart_cfg);
    if (err == -ENOSYS) { /* If invalid configuration */
        printk("uart_configure() error. Invalid configuration\n\r");
        return; 
    }
        
    /* Register callback */
    err = uart_callback_set(uart_dev, uart_cb, NULL);
    if (err) {
        printk("uart_callback_set() error. Error code:%d\n\r",err);
        return;
    }
		
    /* Enable data reception */
    err =  uart_rx_enable(uart_dev ,rx_buf,sizeof(rx_buf),RX_TIMEOUT);
    if (err) {
        printk("uart_rx_enable() error. Error code:%d\n\r",err);
        return;
    }

    /* Send a welcome message */ 
    /* Last arg is timeout. Only relevant if flow controll is used */
    err = uart_tx(uart_dev, welcome_mesg, sizeof(welcome_mesg), SYS_FOREVER_MS);
    if (err) {
        printk("uart_tx() error. Error code:%d\n\r",err);
        return;
    }
	RTDB* db = rtdb_create();

	char buffer[64] = {0};
	int i = 0;
    
    /* Main loop */
    while(1) {
        k_msleep(MAIN_SLEEP_TIME_MS);
        
        /* Print string received so far. */
        /* Very basic implementation, just for showing the use of the API */
        /* E.g. it does not prevent race conditions with the callback!!!!*/
        if(uart_rxbuf_nchar > 0) {
            rx_chars[uart_rxbuf_nchar] = 0; /* Terminate the string */
            uart_rxbuf_nchar = 0;           /* Reset counter */

			buffer[i] = rx_chars[0];

            sprintf(rep_mesg,"You typed [%s]\n\r",rx_chars);

			if(rx_chars[0] == '#'){
                char errCode = '1';
                int k=0;
                char msg[30] = {0};
                while(buffer[i] != '!'){
                    msg[k] = buffer[i];
                    i--,k++;
                    if(k>28) {printk("message too long: %d\n",k), memset(msg,0,30), errCode = '4'; break;}
                    if(i<0) i = 63;
                    if(buffer[i]=='#') {printk("bad message\n"), memset(msg,0,30), errCode = '4';break;}
                }
                msg[k] = '!';
                int j = 0, l = strlen(msg) - 1;
                char temp;
                while(j < l){
                    temp = msg[j];
                    msg[j] = msg[l];
                    msg[l] = temp;
                    l--, j++;
                }
                if(errCode == '4'){
                    char ack[30] = "!1Z";
                    char id[2] = {msg[2], '\0'};
                    strcat(ack,id);
                    strcat(ack,"4");
                    apply_checksum(ack,9);
                    strcat(ack,"\n\r");
                    err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                    if (err) {
                        printk("uart_tx() error. Error code:%d\n\r",err);
                        return;
                    }
                } else if(msg[2] != '0' && msg[2] != '1' && msg[2] != '2' && msg[2] != '3' && msg[2] != '4' && msg[2] != '5' && msg[2] != '6' && msg[2] != '7'){ //é preciso mudar isto
                    printk("msg[2]: %d\n",msg[2]);
                    char ack[30] = "!1Z";
                    char id[2] = {msg[2], '\0'};
                    strcat(ack,id);
                    strcat(ack,"2");
                    apply_checksum(ack,9);
                    strcat(ack,"\n\r");
                    err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                    if (err) {
                        printk("uart_tx() error. Error code:%d\n\r",err);
                        return;
                    }
                } else {
                    if(checkSum(msg)){
                        char ack[30] = "!1Z";
                        char id[2] = {msg[2],'\0'};
                        strcat(ack,id);
                        strcat(ack,"1");
                        apply_checksum(ack,9);
                        strcat(ack,"\n\r");
                        err = uart_tx(uart_dev, ack, sizeof(ack), SYS_FOREVER_MS);
                        if (err) {
                            printk("uart_tx() error. Error code:%d\n\r",err);
                            return;
                        }
                        uart_interface(db, msg);
                    } else {
                        char ack[30] = "!1Z";
                        char id[2] = {msg[2], '\0'};
                        strcat(ack,id);
                        strcat(ack,"3");
                        apply_checksum(ack,9);
                        strcat(ack,"\n\r");
                        err = uart_tx(uart_dev, ack, strlen(ack), SYS_FOREVER_MS);
                        if (err) {
                            printk("uart_tx() error. Error code:%d\n\r",err);
                            return;
                        }
                    }
                    memset(msg,0,30);
                }
			}

            if(i==63) i = 0;
            else i++;
            
            err = uart_tx(uart_dev, rep_mesg, strlen(rep_mesg), SYS_FOREVER_MS);
            if (err) {
                printk("uart_tx() error. Error code:%d\n\r",err);
                return;
            }
        }
        //printk(".\n");
    }
 
}


/* UART callback implementation */
/* Note that callback functions are executed in the scope of interrupt handlers. */
/* They run asynchronously after hardware/software interrupts and have a higher priority than all threads */
/* Should be kept as short and simple as possible. Heavier processing should be deferred to a task with suitable priority*/
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    // int err;

    switch (evt->type) {
	
        case UART_TX_DONE:
		    // printk("UART_TX_DONE event \n\r");
            break;

    	case UART_TX_ABORTED:
	    	// printk("UART_TX_ABORTED event \n\r");
		    break;
		
	    case UART_RX_RDY:
		    // printk("UART_RX_RDY event \n\r");
            /* Just copy data to a buffer. Usually it is preferable to use e.g. a FIFO to communicate with a task that shall process the messages*/
            memcpy(&rx_chars[uart_rxbuf_nchar],&(rx_buf[evt->data.rx.offset]),evt->data.rx.len);
            uart_rxbuf_nchar++;
		    break;

	    case UART_RX_BUF_REQUEST:
		    // printk("UART_RX_BUF_REQUEST event \n\r");
		    break;

	    case UART_RX_BUF_RELEASED:
		    // printk("UART_RX_BUF_RELEASED event \n\r");
		    break;
		
	    case UART_RX_DISABLED: 
            /* When the RX_BUFF becomes full RX is is disabled automaticaly.  */
            /* It must be re-enabled manually for continuous reception */
            // printk("UART_RX_DISABLED event \n\r");
		    // err =  uart_rx_enable(uart_dev ,rx_buf,sizeof(rx_buf),RX_TIMEOUT);
            // if (err) {
            //     printk("uart_rx_enable() error. Error code:%d\n\r",err);
            //     exit(FATAL_ERR);                
            // }
		    break;

	    case UART_RX_STOPPED:
		    // printk("UART_RX_STOPPED event \n\r");
		    break;
		
	    default:
            // printk("UART: unknown event \n\r");
		    break;
    }

}
