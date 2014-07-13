/*
 * BCM2070 Bluetooth driver platform data definition.
 *
 * Author: Niels Langendorff <niels.langendorff@tomtom.com
 * Copyright (c) 2011 TomTom International B.V.
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


#ifndef __BCM2070_H__
#define __BCM2070_H__ 		(__FILE__)

struct bcm2070_platform_data
{
	void (*suspend)(void);
	void (*resume)(void);

	/* Pin definition. */
	void (*config_gpio)(void);
	int (*request_gpio)(void);
	void (*free_gpio)(void);

	void (*reset)(int);
	void (*power)(int);

	/* Used by the rfkill framework */
	struct rfkill *rfk_data;
};

#define BCM2070_DEVNAME			"bcm2070"

#endif /* __BCM2070_H__ */

