/*
 * CSR BlueCore mode driver platform data definition.
 *
 * Author: Mark Vels <mark.vels@tomtom.com>
 * Copyright (c) 2008 TomTom International B.V.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef __ASM_PLATTOMTOM_CSR_BC_MODE_H__
#define __ASM_PLATTOMTOM_CSR_BC_MODE_H__	(__FILE__)

struct csr_bc_info {
	/* Callback for reset pin (when using the default reset function) */
	void (*reset) (int);

	/* Callback to set the CSR_BC mode */
	void (*set_pio0)(int);
	void (*set_pio1)(int);
	void (*set_pio4)(int);

	/* Callback to get the CSR_BC mode */
	int (*get_pio0)(void);
	int (*get_pio1)(void);
	int (*get_pio4)(void);

	/* Callback which overwrites the default reset function */
	void (*do_reset)(struct csr_bc_info *);

	void (*suspend)(void);
	void (*resume)(void);

	/* Callbacks to handle the gpio configuration */
	void (*config_gpio)(void);
	int (*request_gpio)(void);
	void (*free_gpio)(void);

	/* Mode that presented to BC chip */
	volatile int csrbc_mode;

	/* Used by the rfkill framework */
	struct rfkill *rfk_data;
};

#endif /* __ASM_PLATTOMTOM_CSR_BC_MODE_H__ */

