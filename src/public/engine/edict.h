//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined EDICT_H
#define EDICT_H
#ifdef _WIN32
#pragma once
#endif
#ifdef XASH_BUILD
#define MAX_ENT_LEAFS_32	24
#define MAX_ENT_LEAFS_16	48
#else
#define	MAX_ENT_LEAFS	48
#endif

#include "progdefs.h"

struct edict_s
{
	qboolean	free;
	int			serialnumber;
	link_t		area;				// linked to a division node or leaf
	
	int			headnode;			// -1 to use normal leaf check
	int			num_leafs;
#ifdef XASH_BUILD
	union
	{
		int   leafnums32[MAX_ENT_LEAFS_32];
		short leafnums16[MAX_ENT_LEAFS_16];
	};
#else
	short		leafnums[MAX_ENT_LEAFS];
#endif

	float		freetime;			// sv.time when the object was freed

	void*		pvPrivateData;		// Alloced and freed by engine, used by DLLs

	entvars_t	v;					// C exported fields from progs

	// other fields from progs come immediately after
};

#endif
