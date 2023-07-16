/**
 * \file
 *
 * \brief  Main entry point
 *
 */

#include "bms.h"

int main(void) {
	bms_init();
	bms_mainloop();
	//never returns.
}