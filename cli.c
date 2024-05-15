#include <ctype.h>
#include "nrf_cli.h"
#include "nrf_log.h"

#include "boards.h"

#include "client.h"
#include "jammer.h"
#include "master.h"

#define TX_LED BSP_BOARD_LED_2
#define JAMMER_LED BSP_BOARD_LED_3
#define RX_LED BSP_BOARD_LED_0

enum dut_state {
	DUT_STATE_JAMMING,
	DUT_STATE_TX,
	DUT_STATE_RX,
	DUT_STATE_IDLE
};
enum dut_state present_state = DUT_STATE_IDLE;


int jam_start(uint8_t power_level)
{
	if (!jammer_start(power_level)) {
		return 0;
	}
	bsp_board_led_on(JAMMER_LED);
	NRF_LOG_INFO("Jamming started, Power: %d\r\n", power_level);
	return 1;
}

int jam_stop()
{
	jammer_stop();
	bsp_board_led_off(JAMMER_LED);
	NRF_LOG_INFO("Jamming stopped\r\n");
	return 1;
}

int tx_start(uint8_t power_level)
{
	if (!client_start(power_level)) {
		return 0;
	}
	NRF_LOG_INFO("TX started\r\n");
	bsp_board_led_on(TX_LED);
	return 1;
}

int tx_stop()
{
	client_stop();
	NRF_LOG_INFO("TX stopped\r\n");
	bsp_board_led_off(TX_LED);
	return 1;
}

int rx_start()
{
	master_start();
	NRF_LOG_INFO("RX started\r\n");
	bsp_board_led_on(RX_LED);
	return 1;
}

int rx_stop()
{
	master_stop();
	NRF_LOG_INFO("RX stopped\r\n");
	bsp_board_led_off(RX_LED);
	return 1;
}

int dut_set_state(enum dut_state state, int power_level)
{
	if (present_state == state) {
		NRF_LOG_INFO("DUT already in requested state\r\n");
		return 1;
	}

	/* reset state to idle if other state was active  */
	if (present_state != DUT_STATE_IDLE) {
		switch (present_state) {
		case DUT_STATE_JAMMING:
			jam_stop();
			break;
		case DUT_STATE_TX:
			tx_stop();
			break;
		case DUT_STATE_RX:
			rx_stop();
			break;
		default:
			break;
		}
		present_state = DUT_STATE_IDLE;
	}

	/* set requested state */
	switch (state) {
	case DUT_STATE_JAMMING:
		if (!jam_start(power_level)) {
			return 0;
		}
		break;
	case DUT_STATE_TX:
		if (!tx_start(power_level)) {
			return 0;
		}
		break;
	case DUT_STATE_RX:
		rx_start();
		break;
	default:
		break;
	}

	present_state = state;
	return 1;
}

static inline void reset_to_bootloader()
{
	nrf_gpio_cfg_output(19);
}

bool is_int(char *str)
{
    if (*str == '-') {
        str++;
    }
    while (*str) {
        if (!isdigit((unsigned int)*str)) {
            return false;
        }
        str++;
    }
    return true;
}

/* Command handlers */
static void cmd_jam(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (argc < 2) {
        nrf_cli_error(p_cli, "Missing power level");
        return;
    }
    if (!is_int(argv[1])) {
        nrf_cli_error(p_cli, "Invalid power level");
        return;
    }
    dut_set_state(DUT_STATE_JAMMING, atoi(argv[1]));
}

static void cmd_tx(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    if (argc < 2) {
        nrf_cli_error(p_cli, "Missing power level");
        return;
    }
    if (!is_int(argv[1])) {
        nrf_cli_error(p_cli, "Invalid power level");
        return;
    }
    dut_set_state(DUT_STATE_TX, atoi(argv[1]));
}

static void cmd_rx(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    dut_set_state(DUT_STATE_RX, 0);
}

static void cmd_idle(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    dut_set_state(DUT_STATE_IDLE, 0);
}

static void cmd_reset(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    reset_to_bootloader();
}

static void cmd_status(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    switch (present_state) {
    case DUT_STATE_JAMMING:
        nrf_cli_print(p_cli, "JAM");
        break;
    case DUT_STATE_TX:
        nrf_cli_print(p_cli, "TX");
        break;
    case DUT_STATE_RX:
        nrf_cli_print(p_cli, "RX");
        break;
    case DUT_STATE_IDLE:
        nrf_cli_print(p_cli, "Idle");
        break;
    default:
        nrf_cli_print(p_cli, "Unknown");
        break;
    }
}

static void cmd_help(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
    nrf_cli_help_print(p_cli, NULL, 0);
    nrf_cli_print(p_cli, "Commands:\n\n"
                        "jam	- Start jamming\n"
                        "tx	- Start transmitting\n"
                        "rx	- Start receiving\n"
                        "idle	- Idle state\n"
                        "reset	- Reset the board to bootloader\n"
                        "status	- Get device status\n"
                        "help	- Print this help\n");
}

NRF_CLI_CMD_REGISTER(jam, NULL, "Start jamming.", cmd_jam);
NRF_CLI_CMD_REGISTER(tx, NULL, "Start transmitting.", cmd_tx);
NRF_CLI_CMD_REGISTER(rx, NULL, "Start receiving.", cmd_rx);
NRF_CLI_CMD_REGISTER(idle, NULL, "Idle state.", cmd_idle);
NRF_CLI_CMD_REGISTER(reset, NULL, "Reset the board to bootloader.", cmd_reset);
NRF_CLI_CMD_REGISTER(status, NULL, "Get device status.", cmd_status);
NRF_CLI_CMD_REGISTER(help, NULL, "Print help.", cmd_help);
