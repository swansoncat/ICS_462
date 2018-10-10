
#include <vector>

#include "BZDBCache.h"



class ARegion {
	public:
		float x_coord;
		float y_coord;
		float blockWidth;

		float h_value;
		float g_value;
		float f_value;
		ARegion* parent;
		int id;
		bool passable;


};

class ARegionMap {
	public:
		int leftMargin;
		int rightMargin;
		int topMargin;
		int bottomMargin;
		int horizontalTiles;
		int verticalTiles;
		int totalTiles;

		std::vector<ARegion> map;

		float* seekPath(float* target);
	
};