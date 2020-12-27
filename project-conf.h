/*
 * project_conf.h
 *
 *  Created on: 17 Nov 2020
 *      Author: worluk
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define UIP_CONF_TCP 1
#define TI_SPI_CONF_SPI0_ENABLE 1

#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_WARN

#define RPL_CONF_DIS_INTERVAL (5 * CLOCK_SECOND)
#define RPL_CONF_PROBING_INTERVAL (10 * CLOCK_SECOND)

#endif /* PROJECT_CONF_H_ */
