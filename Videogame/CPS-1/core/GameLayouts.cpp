#include "core/RomLayout.h"
#include "core/Games/1941.h"
#include "core/Games/3wonders.h"
#include "core/Games/captcomm.h"
#include "core/Games/cawing.h"
#include "core/Games/ffight.h"
#include "core/Games/knights.h"
#include "core/Games/kod.h"
#include "core/Games/megaman.h"
#include "core/Games/mercs.h"
#include "core/Games/msword.h"
#include "core/Games/mtwins.h"
#include "core/Games/nemo.h"
#include "core/Games/qad.h"
#include "core/Games/sf2.h"
#include "core/Games/sf2ce.h"
#include "core/Games/sf2hf.h"
#include "core/Games/varth.h"
#include <algorithm>
#include <cstring>

// ---------------------------------------------------------------------------
// Aggregator: every known CPS-1 title lives under core/Games/ as its own
// header (namespace cps1::games), one file per game. Adding a title later
// means dropping in a new header, one #include above, and one entry below —
// no manifest, no runtime inference (see docs/IMPLEMENTATION_PLAN.md for why),
// no CMakeLists change (headers are pulled in via #include).
// ---------------------------------------------------------------------------

namespace cps1
{

static constexpr const GameLayout* kAllLayouts[] =
{
    &games::kSf2,
    &games::kSf2Ce,
    &games::kMegaman,
    &games::k1941,
    &games::kFfight,
    &games::kMercs,
    &games::kMtwins,
    &games::kMsword,
    &games::kCawing,
    &games::kNemo,
    &games::k3wonders,
    &games::kKod,
    &games::kCaptcomm,
    &games::kKnights,
    &games::kVarth,
    &games::kQad,
    &games::kSf2hf,
};

const GameLayout* findGameLayout(const char* id)
{
    const auto it = std::find_if(std::begin(kAllLayouts), std::end(kAllLayouts),
                                  [id](const GameLayout* layout)
                                  { return std::strcmp(layout->id, id) == 0; });
    return it != std::end(kAllLayouts) ? *it : nullptr;
}

} // namespace cps1
