/*
 * Copyright (c) 2022-2024 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NAND_BCH_H__
#define NAND_BCH_H__

#include "lib/spi_nand/spi_nand.h"
#include "lib/spi_nand/bitops.h"

struct nand_bch_control;

#define CONFIG_NAND_ECC_BCH

static inline int mtd_nand_has_bch(void) { return 1; }

/*
 * Calculate BCH ecc code
 */
int nand_bch_calculate_ecc(struct nand_dev *nand, const unsigned char *dat,
		unsigned char *ecc_code);

/*
 * Detect and correct bit errors
 */
int nand_bch_correct_data(struct nand_dev *nand, unsigned char *dat, unsigned char *read_ecc,
		unsigned char *calc_ecc);
/*
 * Initialize BCH encoder/decoder
 */
struct nand_bch_control *
nand_bch_init(struct nand_dev *nand, unsigned int eccsize,
	      unsigned int eccbytes, struct NandEccLayout **ecclayout);
/*
 * Release BCH encoder/decoder resources
 */
void nand_bch_free(struct nand_bch_control *nbc);


int nand_read_ecc(struct nand_dev *nand, uint32_t page, uint8_t *buf, uint32_t len);

int nand_read_ondie(struct nand_dev *nand, uint32_t page, uint8_t *buf, uint32_t len);

int nand_write_ecc(struct nand_dev *nand, uint32_t page, uint8_t *buf, uint32_t len);

int nand_write_ondie(struct nand_dev *nand, uint32_t page, uint8_t *buf, uint32_t len);

/*
 * Function:      Nand_Ecc_Init
 * Arguments:     chip,    pointer to an mxchip structure of SPI NAND device.
 *                EccMode, ECC mode.
 * Return Value:  RET_OK.
 *                RET_ERR.
 * Description:   This function initializes the software variables related
 *                to ECC generation, ECC checking and writing ECC Bytes in spare Bytes.
 */
int Nand_Ecc_Init(struct nand_dev *nd, uint32_t EccMode);


#endif /* __MTD_NAND_BCH_H__ */
