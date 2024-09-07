// Copied from libtonc
// https://github.com/devkitPro/libtonc/blob/ccc03fa321e56f51aed5e2ee1d6e3df3d1cbc803/include/tonc_memdef.h#L245

#ifndef TONC_BLD
#define TONC_BLD

// --- REG_BLDCNT ------------------------------------------------------

/*!	\defgroup grpVideoBLD	Blend Flags
	\ingroup grpMemBits
	\brief	Macros for REG_BLDCNT, REG_BLDY and REG_BLDALPHA
*/
/*!	\{	*/

//!\ name Blend control
//\{

#define BLD_BG0			0x0001	//!< Blend bg 0
#define BLD_BG1			0x0002	//!< Blend bg 1
#define BLD_BG2			0x0004	//!< Blend bg 2
#define BLD_BG3			0x0008	//!< Blend bg 3
#define BLD_OBJ			0x0010	//!< Blend objects
#define BLD_ALL			0x001F	//!< All layers (except backdrop)
#define BLD_BACKDROP	0x0020	//!< Blend backdrop
#define BLD_OFF				 0	//!< Blend mode is off
#define BLD_STD			0x0040	//!< Normal alpha blend (with REG_EV)
#define BLD_WHITE		0x0080	//!< Fade to white (with REG_Y)
#define BLD_BLACK		0x00C0	//!< Fade to black (with REG_Y)

#define BLD_TOP_MASK	0x003F
#define BLD_TOP_SHIFT		 0
#define BLD_TOP(n)		((n)<<BLD_TOP_SHIFT)

#define BLD_MODE_MASK	0x00C0
#define BLD_MODE_SHIFT		 6
#define BLD_MODE(n)		((n)<<BLD_MODE_SHIFT)

#define BLD_BOT_MASK	0x3F00
#define BLD_BOT_SHIFT		 8
#define BLD_BOT(n)		((n)<<BLD_BOT_SHIFT)

#define BLD_BUILD(top, bot, mode)		\
	( (((bot)&63)<<8) | (((mode)&3)<<6) | ((top)&63) )

//\}

// --- REG_BLDALPHA ---

//! \name Blend weights

#define BLD_EVA_MASK	0x001F
#define BLD_EVA_SHIFT		 0
#define BLD_EVA(n)		((n)<<BLD_EVA_SHIFT)

#define BLD_EVB_MASK	0x1F00
#define BLD_EVB_SHIFT		 8
#define BLD_EVB(n)		((n)<<BLD_EVB_SHIFT)

#define BLDA_BUILD(eva, evb)		\
	( ((eva)&31) | (((evb)&31)<<8) )

//\}


// --- REG_BLDY ---

//! \name Fade levels

#define BLDY_MASK		0x001F
#define BLDY_SHIFT		 0
#define BLDY(n)		((n)<<BLD_EY_SHIFT)

#define BLDY_BUILD(ey)				\
	( (ey)&31 )

//\}

/*!	\}	*/

#endif
