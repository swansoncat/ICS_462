

#include <vector>
#include <string>

#include "BZDBCache.h"



class ARegion {
	public:
		ARegion();
		ARegion(float x, float y, int i, bool b);
		static int getCurrentTile(float* f);
		void addNeighbor(ARegion a);
		float* getTileCoordinates();
		bool operator< (const ARegion &other) const {return g_value < other.g_value;}

		float x_coord;
		float y_coord;
		float blockWidth;

		float h_value;
		float g_value;
		float f_value;
		ARegion* parent;
		std::vector<ARegion> neighbors;
		std::vector<int> neighborTileNum;
		int id;
		bool passable;
		
		float testFloat;
		int testInt;
		std::string testString;

};

class ARegionMap {
	public:
		ARegionMap();
		ARegionMap(bool b);
		int setTarget(float* pos, float* target);
		float* getPosCoordinates(int i);
		std::vector<int> getNeighborTiles(int i);

		int leftMargin;
		int rightMargin;
		int topMargin;
		int bottomMargin;
		int horizontalTiles;
		int verticalTiles;
		int totalTiles;

		std::vector<ARegion> map;
		std::vector<ARegion>::iterator iter;


	
};
