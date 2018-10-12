
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

int ARegion::getCurrentTile(float* f)
{	
	int currentTile = -1;
	int currentX = -1;
	int currentY = -1;
	int mapSideLength = BZDBCache::worldSize;

	float x = f[0];
	float y = f[1];

	currentX = ceil((f[0] - (0 - (mapSideLength / 2))) / BZDBCache::tankRadius);
	currentY = ceil(((0 + (mapSideLength / 2)) - f[1]) / BZDBCache::tankRadius);
	
	currentTile = (ceil(mapSideLength / BZDBCache::tankRadius) * currentY) + currentX;

	return currentTile;
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
	horizontalTiles = ceil(BZDBCache::worldSize / BZDBCache::tankRadius);
	//horizontalTiles = BZDBCache::worldSize;
	verticalTiles = ceil(BZDBCache::worldSize / BZDBCache::tankRadius);
	//verticalTiles = squareRoot;
	totalTiles = horizontalTiles * verticalTiles;

	
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
				map.push_back(a);
			}
			else
			{
				ARegion a = ARegion(xCenter, yCenter, count, true);
				map.push_back(a);
			}
			
		}
		count++;
	}		
	iter = map.begin();


}

//should be return float* but changing to int for testing
int ARegionMap::setTarget(float* target)
{
	
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
		tiles[k].h_value = xOffset + yOffset;
		map.at(k).h_value = xOffset + yOffset;
	}
	return totalTiles;
}








