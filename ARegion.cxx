
#include <vector>
#include "ARegion.h"
#include "World.h"

ARegion::ARegion()
{
	//do nothing
}

ARegion::ARegion(float x, float y, int i, bool b)
{
	x_coord = x;
	y_coord = y;
	id = i;
	passable = b;
	
}


ARegionMap::ARegionMap()
{
	//do nothing
}

ARegionMap::ARegionMap(bool b)
{
	//double squareRoot = sqrt(BZDBCache::worldSize);
	int half = BZDBCache::worldSize / 2;
	leftMargin = 0 - half;
	rightMargin = 0 + half;
	topMargin = 0 + half;
	bottomMargin = 0 - half;
	horizontalTiles = ceil(half / BZDBCache::tankRadius);
	//horizontalTiles = BZDBCache::worldSize;
	verticalTiles = ceil(half / BZDBCache::tankRadius);
	//verticalTiles = squareRoot;
	totalTiles = horizontalTiles * verticalTiles;
	iter = map.begin();
	
	for (int i = 0 ; i < verticalTiles ; i++)
	{
		int count = 1;
		for (int j = 0 ; j < horizontalTiles ; j++)
		{
			float tankHalf = BZDBCache::tankRadius / 2;
			float xCenter = leftMargin + tankHalf + (j * BZDBCache::tankRadius);
			float yCenter = topMargin - tankHalf - (i * BZDBCache::tankRadius);
			float coord[3];
			coord[0] = xCenter;
			coord[1] = yCenter;
			coord[2] = 0;
			
			if (World::getWorld()->inBuilding(coord, tankHalf, 0) != NULL)
			{
				ARegion a = ARegion(xCenter, yCenter, count, false);
				//ARegion aa = *(a + 0);
				map.push_back(a);
				iter++;	
			}
			else
			{
				ARegion a = ARegion(xCenter, yCenter, count, true);
				//ARegion aa = *(a + 0);
				map.push_back(a);
				iter++;	
			}
			
		}
		count++;
	}		
	


}

//should be return float* but changing to int for testing
int ARegionMap::setTarget(float* target)
{
	/*
	ARegion tiles[totalTiles];
	for (int n = 0 ; n < map.size() ; n++)
	{
		//std::vector<ARegion>::iterator tileIter = map.beginning():
		tiles[n] = map.at(n);
	}
	

	float xTarg = target[0];
	float yTarg = target[1];

	int targetTile = 0;
	int targetTileX = ceil((xTarg - leftMargin) / BZDBCache::tankRadius);
	int targetTileY = (topMargin - yTarg) / BZDBCache::tankRadius;

	targetTileY = verticalTiles * targetTileY;
	targetTile = targetTileX + targetTileY;
	for (int k = 0 ; k < totalTiles ; k++)
	{
		int posTileX = ceil((map.at(k).x_coord - leftMargin) / BZDBCache::tankRadius);
		int posTileY = (topMargin - (map.at(k).x_coord)) / BZDBCache::tankRadius;
		int xOffset = targetTileX - posTileX;
		int yOffset = targetTileY - posTileY;
		//tiles[k].h_value = xOffset + yOffset;
		map.at(k).h_value = xOffset + yOffset;
	}*/
	return totalTiles;
}






