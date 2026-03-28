/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef USERCMD_H
#define USERCMD_H
#ifdef _WIN32
#pragma once
#endif

typedef struct usercmd_s
{
#ifdef XASH_BUILD
	int16_t   lerp_msec;
	int8_t    msec;
	uint8_t   pad1;
	vec3_t    viewangles;

	float     forwardmove;
	float     sidemove;
	float     upmove;
	uint8_t   lightlevel;
	uint8_t   pad2;
	uint16_t  buttons;
	uint8_t   impulse;
	uint8_t   weaponselect;
	uint8_t   pad3[2];

	int32_t   reserved[4]; // unused HL impact stuff, left for modders
#else
	short lerp_msec;   // Interpolation time on client
	byte msec;		   // Duration in ms of command
	vec3_t viewangles; // Command view angles.

	// intended velocities
	float forwardmove;		// Forward velocity.
	float sidemove;			// Sideways velocity.
	float upmove;			// Upward velocity.
	byte lightlevel;		// Light level at spot where we are standing.
	unsigned short buttons; // Attack buttons
	byte impulse;			// Impulse command issued.
	byte weaponselect;		// Current weapon id

	// Experimental player impact stuff.
	int impact_index;
	vec3_t impact_position;
#endif
} usercmd_t;

#endif // USERCMD_H
