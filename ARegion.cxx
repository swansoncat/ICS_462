
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

/* This function takes a float* as an argument, which should be a pair of coordinates of somewhere on the map. The function returns the tile corresponding to those coordinates.
*
*/
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

void ARegion::addNeighbor(ARegion a)
{

}


ARegionMap::ARegionMap()
{
	//do nothing
}
/*	This creates a tiled map of the game map, which will be used to do the A-star search.
*
*
*/
ARegionMap::ARegionMap(bool b)
{
	int half = BZDBCache::worldSize / 2;
	leftMargin = 0 - half;
	rightMargin = 0 + half;
	topMargin = 0 + half;
	bottomMargin = 0 - half;
	horizontalTiles = ceil(BZDBCache::worldSize / BZDBCache::tankRadius);
	verticalTiles = ceil(BZDBCache::worldSize / BZDBCache::tankRadius);
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

/*should be return float* but changing to int for testing
*
* Currently this function just sets all the h-values for each tile in the A-star search map. 
*/
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

std::vector<int> ARegionMap::getNeighborTiles(int i)
{
	std::vector<int> numbers;
	//first tile, upper left corner of map
	if (i % horizontalTiles == 1 && i == 1)
	{
		numbers.push_back(2);
		numbers.push_back(horizontalTiles + 1);
		numbers.push_back(horizontalTiles + 2);		
	}
	//upper right corner of map
	else if(i % horizontalTiles == 0 && i == horizontalTiles)
	{
		numbers.push_back(horizontalTiles - 1);
		numbers.push_back(horizontalTiles - 1 + horizontalTiles);
		numbers.push_back(horizontalTiles * 2);
	}
	//bottom left corner of map
	else if (i % horizontalTiles == 1 && i == totalTiles + 1 - horizontalTiles)
	{
		numbers.push_back(totalTiles + 2 - horizontalTiles);
		numbers.push_back(totalTiles - 2 - horizontalTiles);	
		numbers.push_back(totalTiles - 3 - horizontalTiles);	
	}
	//bottom right corner of map
	else if (i % horizontalTiles == 0 && i == totalTiles)
	{
		numbers.push_back(totalTiles - 1);
		numbers.push_back(totalTiles - horizontalTiles);	
		numbers.push_back(totalTiles - 1 - horizontalTiles);	
	}
	//left edge of map that isn't a corner
	else if (i % horizontalTiles == 1 && i > horizontalTiles && i < totalTiles - horizontalTiles)
	{
		numbers.push_back(i + 1);
		numbers.push_back(i - horizontalTiles);
		numbers.push_back(i + horizontalTiles);
		numbers.push_back(i - horizontalTiles + 1);
		numbers.push_back(i + horizontalTiles - 1);    
	}
	//right edge of map that isn't a corner
	else if (i % horizontalTiles == 0 && i > horizontalTiles && i < totalTiles - horizontalTiles + 1)
	{
		numbers.push_back(i - 1);
		numbers.push_back(i - horizontalTiles);
		numbers.push_back(i + horizontalTiles);
		numbers.push_back(i - horizontalTiles - 1);
		numbers.push_back(i + horizontalTiles - 1);    
	}
	//top edge of map that isn't a corner
	else if(i != 1 && i != horizontalTiles && 1 < i < horizontalTiles)
	{
		numbers.push_back(i - 1);
		numbers.push_back(i + 1);
		numbers.push_back(i + horizontalTiles);
		numbers.push_back(i + horizontalTiles + 1);
		numbers.push_back(i + horizontalTiles - 1);
	}
	//bottom edge of map that isn't a corner
	else if(i != totalTiles && i != totalTiles + 1 - horizontalTiles && totalTiles > i > totalTiles + 1 - horizontalTiles)
	{
		numbers.push_back(i - 1);
		numbers.push_back(i + 1);
		numbers.push_back(i - horizontalTiles);
		numbers.push_back(i - horizontalTiles + 1);
		numbers.push_back(i - horizontalTiles - 1);
	}
	//center tile, or one surrounded by 8 neighbors
	else
	{
		numbers.push_back(i - 1);
		numbers.push_back(i + 1);
		numbers.push_back(i - horizontalTiles);
		numbers.push_back(i - horizontalTiles + 1);
		numbers.push_back(i - horizontalTiles - 1);
		numbers.push_back(i + horizontalTiles);
		numbers.push_back(i + horizontalTiles + 1);
		numbers.push_back(i + horizontalTiles - 1);
	}
	return numbers;
}








