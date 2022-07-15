#include "Interfaces.h"

using namespace PaintsNow;

Interfaces::Interfaces(IArchive& parchive, IAudio& paudio, IDatabase& pdatabase, 
	IFilterBase& passetFilterBase, IFilterBase& paudioFilterBase, IFontBase& pfontBase, IFrame& pframe, IImage& pimage, 
	INetwork& pstandardNetwork, INetwork& pquickNetwork, IRandom& prandom, IRender& prender,
	IScript& pscript, IThread& pthread, ITimer& ptimer)
	: archive(parchive), audio(paudio), database(pdatabase), 
	assetFilterBase(passetFilterBase), audioFilterBase(paudioFilterBase), fontBase(pfontBase),
	frame(pframe), image(pimage), standardNetwork(pstandardNetwork), quickNetwork(pquickNetwork), random(prandom), render(prender), script(pscript),
	thread(pthread), timer(ptimer) {}