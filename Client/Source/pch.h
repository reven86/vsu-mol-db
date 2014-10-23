#ifndef __DFG_PCH__
#define __DFG_PCH__

/*!	\file pch.h 
 *	\brief Precompiled header.
 */



#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <numeric>
#include <iterator>
#include <tuple>
#include <unordered_map>
#include <random>


#pragma warning( disable: 4100 )	// unreferenced formal parameter
#pragma warning( disable: 4127 )	// conditional expression is constant
#pragma warning( disable: 4201 )	// nameless struct/union

#pragma warning( push )
#pragma warning( disable: 4100 )
#include "gameplay.h"
#include "mathutil.h"
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable: 4512 )
#include "sigc++.h"
#pragma warning( pop )



#include "utils/ref_ptr.h"
#include "utils/noncopyable.h"
#include "utils/singleton.h"
#include "utils/utils.h"
#include "utils/curve.h"
#include "utils/profiler.h"
#include "main/asset.h"
#include "main/cache.h"
#include "main/gameplay_assets.h"
#include "math/transformable.h"
#include "main/dictionary.h"

#include "game/signals.h"
#include "game/settings.h"

extern Dictionary gameDictionary;       // global dictionary.
extern Settings * getSettings();


#endif // __DFG_PCH__